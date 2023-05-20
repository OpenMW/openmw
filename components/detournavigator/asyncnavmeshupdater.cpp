#include "asyncnavmeshupdater.hpp"
#include "dbrefgeometryobject.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "navmeshdbutils.hpp"
#include "serialization.hpp"
#include "settings.hpp"
#include "version.hpp"

#include <components/debug/debuglog.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/misc/thread.hpp>

#include <DetourNavMesh.h>

#include <osg/io_utils>

#include <algorithm>
#include <numeric>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>

namespace DetourNavigator
{
    namespace
    {
        int getManhattanDistance(const TilePosition& lhs, const TilePosition& rhs)
        {
            return std::abs(lhs.x() - rhs.x()) + std::abs(lhs.y() - rhs.y());
        }

        bool isAbsentTileTooClose(const TilePosition& position, int distance,
            const std::set<std::tuple<AgentBounds, TilePosition>>& pushedTiles,
            const std::set<std::tuple<AgentBounds, TilePosition>>& presentTiles,
            const Misc::ScopeGuarded<std::set<std::tuple<AgentBounds, TilePosition>>>& processingTiles)
        {
            const auto isAbsentAndCloserThan = [&](const std::tuple<AgentBounds, TilePosition>& v) {
                return presentTiles.find(v) == presentTiles.end()
                    && getManhattanDistance(position, std::get<1>(v)) < distance;
            };
            if (std::any_of(pushedTiles.begin(), pushedTiles.end(), isAbsentAndCloserThan))
                return true;
            if (const auto locked = processingTiles.lockConst();
                std::any_of(locked->begin(), locked->end(), isAbsentAndCloserThan))
                return true;
            return false;
        }

        auto getPriority(const Job& job) noexcept
        {
            return std::make_tuple(-static_cast<std::underlying_type_t<JobState>>(job.mState), job.mProcessTime,
                job.mChangeType, job.mTryNumber, job.mDistanceToPlayer, job.mDistanceToOrigin);
        }

        struct LessByJobPriority
        {
            bool operator()(JobIt lhs, JobIt rhs) const noexcept { return getPriority(*lhs) < getPriority(*rhs); }
        };

        void insertPrioritizedJob(JobIt job, std::deque<JobIt>& queue)
        {
            const auto it = std::upper_bound(queue.begin(), queue.end(), job, LessByJobPriority{});
            queue.insert(it, job);
        }

        auto getDbPriority(const Job& job) noexcept
        {
            return std::make_tuple(static_cast<std::underlying_type_t<JobState>>(job.mState), job.mChangeType,
                job.mDistanceToPlayer, job.mDistanceToOrigin);
        }

        struct LessByJobDbPriority
        {
            bool operator()(JobIt lhs, JobIt rhs) const noexcept { return getDbPriority(*lhs) < getDbPriority(*rhs); }
        };

        void insertPrioritizedDbJob(JobIt job, std::deque<JobIt>& queue)
        {
            const auto it = std::upper_bound(queue.begin(), queue.end(), job, LessByJobDbPriority{});
            queue.insert(it, job);
        }

        auto getAgentAndTile(const Job& job) noexcept
        {
            return std::make_tuple(job.mAgentBounds, job.mChangedTile);
        }

        std::unique_ptr<DbWorker> makeDbWorker(
            AsyncNavMeshUpdater& updater, std::unique_ptr<NavMeshDb>&& db, const Settings& settings)
        {
            if (db == nullptr)
                return nullptr;
            return std::make_unique<DbWorker>(updater, std::move(db), TileVersion(navMeshFormatVersion),
                settings.mRecast, settings.mWriteToNavMeshDb);
        }

        void updateJobs(std::deque<JobIt>& jobs, TilePosition playerTile, int maxTiles)
        {
            for (JobIt job : jobs)
            {
                job->mDistanceToPlayer = getManhattanDistance(job->mChangedTile, playerTile);
                if (!shouldAddTile(job->mChangedTile, playerTile, maxTiles))
                    job->mChangeType = ChangeType::remove;
            }
        }

        std::size_t getNextJobId()
        {
            static std::atomic_size_t nextJobId{ 1 };
            return nextJobId.fetch_add(1);
        }

        bool isWritingDbJob(const Job& job)
        {
            return job.mGeneratedNavMeshData != nullptr;
        }
    }

    std::ostream& operator<<(std::ostream& stream, JobStatus value)
    {
        switch (value)
        {
            case JobStatus::Done:
                return stream << "JobStatus::Done";
            case JobStatus::Fail:
                return stream << "JobStatus::Fail";
            case JobStatus::MemoryCacheMiss:
                return stream << "JobStatus::MemoryCacheMiss";
        }
        return stream << "JobStatus::" << static_cast<std::underlying_type_t<JobStatus>>(value);
    }

    Job::Job(const AgentBounds& agentBounds, std::weak_ptr<GuardedNavMeshCacheItem> navMeshCacheItem,
        std::string_view worldspace, const TilePosition& changedTile, ChangeType changeType, int distanceToPlayer,
        std::chrono::steady_clock::time_point processTime)
        : mId(getNextJobId())
        , mAgentBounds(agentBounds)
        , mNavMeshCacheItem(std::move(navMeshCacheItem))
        , mWorldspace(worldspace)
        , mChangedTile(changedTile)
        , mProcessTime(processTime)
        , mChangeType(changeType)
        , mDistanceToPlayer(distanceToPlayer)
        , mDistanceToOrigin(getManhattanDistance(changedTile, TilePosition{ 0, 0 }))
    {
    }

    AsyncNavMeshUpdater::AsyncNavMeshUpdater(const Settings& settings, TileCachedRecastMeshManager& recastMeshManager,
        OffMeshConnectionsManager& offMeshConnectionsManager, std::unique_ptr<NavMeshDb>&& db)
        : mSettings(settings)
        , mRecastMeshManager(recastMeshManager)
        , mOffMeshConnectionsManager(offMeshConnectionsManager)
        , mShouldStop()
        , mNavMeshTilesCache(settings.mMaxNavMeshTilesCacheSize)
        , mDbWorker(makeDbWorker(*this, std::move(db), mSettings))
    {
        for (std::size_t i = 0; i < mSettings.get().mAsyncNavMeshUpdaterThreads; ++i)
            mThreads.emplace_back([&] { process(); });
    }

    AsyncNavMeshUpdater::~AsyncNavMeshUpdater()
    {
        stop();
    }

    void AsyncNavMeshUpdater::post(const AgentBounds& agentBounds, const SharedNavMeshCacheItem& navMeshCacheItem,
        const TilePosition& playerTile, std::string_view worldspace,
        const std::map<TilePosition, ChangeType>& changedTiles)
    {
        bool playerTileChanged = false;
        {
            auto locked = mPlayerTile.lock();
            playerTileChanged = *locked != playerTile;
            *locked = playerTile;
        }

        if (!playerTileChanged && changedTiles.empty())
            return;

        const dtNavMeshParams params = *navMeshCacheItem->lockConst()->getImpl().getParams();
        const int maxTiles = std::min(mSettings.get().mMaxTilesNumber, params.maxTiles);

        std::unique_lock lock(mMutex);

        if (playerTileChanged)
            updateJobs(mWaiting, playerTile, maxTiles);

        for (const auto& [changedTile, changeType] : changedTiles)
        {
            if (mPushed.emplace(agentBounds, changedTile).second)
            {
                const auto processTime = changeType == ChangeType::update
                    ? mLastUpdates[std::tie(agentBounds, changedTile)] + mSettings.get().mMinUpdateInterval
                    : std::chrono::steady_clock::time_point();

                const JobIt it = mJobs.emplace(mJobs.end(), agentBounds, navMeshCacheItem, worldspace, changedTile,
                    changeType, getManhattanDistance(changedTile, playerTile), processTime);

                Log(Debug::Debug) << "Post job " << it->mId << " for agent=(" << it->mAgentBounds << ")"
                                  << " changedTile=(" << it->mChangedTile << ") "
                                  << " changeType=" << it->mChangeType;

                if (playerTileChanged)
                    mWaiting.push_back(it);
                else
                    insertPrioritizedJob(it, mWaiting);
            }
        }

        if (playerTileChanged)
            std::sort(mWaiting.begin(), mWaiting.end(), LessByJobPriority{});

        Log(Debug::Debug) << "Posted " << mJobs.size() << " navigator jobs";

        if (!mWaiting.empty())
            mHasJob.notify_all();

        lock.unlock();

        if (playerTileChanged && mDbWorker != nullptr)
            mDbWorker->updateJobs(playerTile, maxTiles);
    }

    void AsyncNavMeshUpdater::wait(WaitConditionType waitConditionType, Loading::Listener* listener)
    {
        switch (waitConditionType)
        {
            case WaitConditionType::requiredTilesPresent:
                waitUntilJobsDoneForNotPresentTiles(listener);
                break;
            case WaitConditionType::allJobsDone:
                waitUntilAllJobsDone();
                break;
        }
    }

    void AsyncNavMeshUpdater::stop()
    {
        mShouldStop = true;
        if (mDbWorker != nullptr)
            mDbWorker->stop();
        std::unique_lock<std::mutex> lock(mMutex);
        mWaiting.clear();
        mHasJob.notify_all();
        lock.unlock();
        for (auto& thread : mThreads)
            if (thread.joinable())
                thread.join();
    }

    void AsyncNavMeshUpdater::waitUntilJobsDoneForNotPresentTiles(Loading::Listener* listener)
    {
        const int maxDistanceToPlayer = mSettings.get().mWaitUntilMinDistanceToPlayer;
        if (maxDistanceToPlayer <= 0)
            return;
        const std::size_t initialJobsLeft = getTotalJobs();
        std::size_t maxProgress = initialJobsLeft;
        std::size_t prevJobsLeft = initialJobsLeft;
        std::size_t jobsDone = 0;
        std::size_t jobsLeft = 0;
        const TilePosition playerPosition = *mPlayerTile.lockConst();
        const auto isDone = [&] {
            jobsLeft = mJobs.size();
            if (jobsLeft == 0)
                return true;
            return !isAbsentTileTooClose(playerPosition, maxDistanceToPlayer, mPushed, mPresentTiles, mProcessingTiles);
        };
        std::unique_lock<std::mutex> lock(mMutex);
        if (!isAbsentTileTooClose(playerPosition, maxDistanceToPlayer, mPushed, mPresentTiles, mProcessingTiles)
            || mJobs.empty())
            return;
        const Loading::ScopedLoad load(listener);
        if (listener != nullptr)
        {
            listener->setLabel("#{OMWEngine:BuildingNavigationMesh}");
            listener->setProgressRange(maxProgress);
        }
        while (!mDone.wait_for(lock, std::chrono::milliseconds(20), isDone))
        {
            if (listener == nullptr)
                continue;
            if (maxProgress < jobsLeft)
            {
                maxProgress = jobsLeft;
                listener->setProgressRange(maxProgress);
                listener->setProgress(jobsDone);
            }
            else if (jobsLeft < prevJobsLeft)
            {
                const std::size_t newJobsDone = prevJobsLeft - jobsLeft;
                jobsDone += newJobsDone;
                prevJobsLeft = jobsLeft;
                listener->increaseProgress(newJobsDone);
            }
        }
    }

    void AsyncNavMeshUpdater::waitUntilAllJobsDone()
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mDone.wait(lock, [this] { return mJobs.empty(); });
        }
        mProcessingTiles.wait(mProcessed, [](const auto& v) { return v.empty(); });
    }

    AsyncNavMeshUpdaterStats AsyncNavMeshUpdater::getStats() const
    {
        AsyncNavMeshUpdaterStats result;
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            result.mJobs = mJobs.size();
            result.mWaiting = mWaiting.size();
            result.mPushed = mPushed.size();
        }
        result.mProcessing = mProcessingTiles.lockConst()->size();
        if (mDbWorker != nullptr)
            result.mDb = mDbWorker->getStats();
        result.mCache = mNavMeshTilesCache.getStats();
        result.mDbGetTileHits = mDbGetTileHits.load(std::memory_order_relaxed);
        return result;
    }

    void AsyncNavMeshUpdater::process() noexcept
    {
        Log(Debug::Debug) << "Start process navigator jobs by thread=" << std::this_thread::get_id();
        Misc::setCurrentThreadIdlePriority();
        while (!mShouldStop)
        {
            try
            {
                if (JobIt job = getNextJob(); job != mJobs.end())
                {
                    const JobStatus status = processJob(*job);
                    Log(Debug::Debug) << "Processed job " << job->mId << " with status=" << status;
                    switch (status)
                    {
                        case JobStatus::Done:
                            unlockTile(job->mAgentBounds, job->mChangedTile);
                            if (job->mGeneratedNavMeshData != nullptr)
                                mDbWorker->enqueueJob(job);
                            else
                                removeJob(job);
                            break;
                        case JobStatus::Fail:
                            repost(job);
                            break;
                        case JobStatus::MemoryCacheMiss:
                        {
                            mDbWorker->enqueueJob(job);
                            break;
                        }
                    }
                }
                else
                    cleanupLastUpdates();
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "AsyncNavMeshUpdater::process exception: " << e.what();
            }
        }
        Log(Debug::Debug) << "Stop navigator jobs processing by thread=" << std::this_thread::get_id();
    }

    JobStatus AsyncNavMeshUpdater::processJob(Job& job)
    {
        Log(Debug::Debug) << "Processing job " << job.mId << " by thread=" << std::this_thread::get_id();

        const auto navMeshCacheItem = job.mNavMeshCacheItem.lock();

        if (!navMeshCacheItem)
            return JobStatus::Done;

        const auto playerTile = *mPlayerTile.lockConst();
        const auto params = *navMeshCacheItem->lockConst()->getImpl().getParams();

        if (!shouldAddTile(job.mChangedTile, playerTile, std::min(mSettings.get().mMaxTilesNumber, params.maxTiles)))
        {
            Log(Debug::Debug) << "Ignore add tile by job " << job.mId << ": too far from player";
            navMeshCacheItem->lock()->removeTile(job.mChangedTile);
            return JobStatus::Done;
        }

        switch (job.mState)
        {
            case JobState::Initial:
                return processInitialJob(job, *navMeshCacheItem);
            case JobState::WithDbResult:
                return processJobWithDbResult(job, *navMeshCacheItem);
        }

        return JobStatus::Done;
    }

    JobStatus AsyncNavMeshUpdater::processInitialJob(Job& job, GuardedNavMeshCacheItem& navMeshCacheItem)
    {
        Log(Debug::Debug) << "Processing initial job " << job.mId;

        std::shared_ptr<RecastMesh> recastMesh = mRecastMeshManager.get().getMesh(job.mWorldspace, job.mChangedTile);

        if (recastMesh == nullptr)
        {
            Log(Debug::Debug) << "Null recast mesh for job " << job.mId;
            navMeshCacheItem.lock()->markAsEmpty(job.mChangedTile);
            return JobStatus::Done;
        }

        if (isEmpty(*recastMesh))
        {
            Log(Debug::Debug) << "Empty bounds for job " << job.mId;
            navMeshCacheItem.lock()->markAsEmpty(job.mChangedTile);
            return JobStatus::Done;
        }

        NavMeshTilesCache::Value cachedNavMeshData
            = mNavMeshTilesCache.get(job.mAgentBounds, job.mChangedTile, *recastMesh);
        std::unique_ptr<PreparedNavMeshData> preparedNavMeshData;
        const PreparedNavMeshData* preparedNavMeshDataPtr = nullptr;

        if (cachedNavMeshData)
        {
            preparedNavMeshDataPtr = &cachedNavMeshData.get();
        }
        else
        {
            if (job.mChangeType != ChangeType::update && mDbWorker != nullptr)
            {
                job.mRecastMesh = std::move(recastMesh);
                return JobStatus::MemoryCacheMiss;
            }

            preparedNavMeshData
                = prepareNavMeshTileData(*recastMesh, job.mChangedTile, job.mAgentBounds, mSettings.get().mRecast);

            if (preparedNavMeshData == nullptr)
            {
                Log(Debug::Debug) << "Null navmesh data for job " << job.mId;
                navMeshCacheItem.lock()->markAsEmpty(job.mChangedTile);
                return JobStatus::Done;
            }

            if (job.mChangeType == ChangeType::update)
            {
                preparedNavMeshDataPtr = preparedNavMeshData.get();
            }
            else
            {
                cachedNavMeshData = mNavMeshTilesCache.set(
                    job.mAgentBounds, job.mChangedTile, *recastMesh, std::move(preparedNavMeshData));
                preparedNavMeshDataPtr = cachedNavMeshData ? &cachedNavMeshData.get() : preparedNavMeshData.get();
            }
        }

        const auto offMeshConnections = mOffMeshConnectionsManager.get().get(job.mChangedTile);

        const UpdateNavMeshStatus status
            = navMeshCacheItem.lock()->updateTile(job.mChangedTile, std::move(cachedNavMeshData),
                makeNavMeshTileData(*preparedNavMeshDataPtr, offMeshConnections, job.mAgentBounds, job.mChangedTile,
                    mSettings.get().mRecast));

        return handleUpdateNavMeshStatus(status, job, navMeshCacheItem, *recastMesh);
    }

    JobStatus AsyncNavMeshUpdater::processJobWithDbResult(Job& job, GuardedNavMeshCacheItem& navMeshCacheItem)
    {
        Log(Debug::Debug) << "Processing job with db result " << job.mId;

        std::unique_ptr<PreparedNavMeshData> preparedNavMeshData;
        bool generatedNavMeshData = false;

        if (job.mCachedTileData.has_value() && job.mCachedTileData->mVersion == navMeshFormatVersion)
        {
            preparedNavMeshData = std::make_unique<PreparedNavMeshData>();
            if (deserialize(job.mCachedTileData->mData, *preparedNavMeshData))
                ++mDbGetTileHits;
            else
                preparedNavMeshData = nullptr;
        }

        if (preparedNavMeshData == nullptr)
        {
            preparedNavMeshData
                = prepareNavMeshTileData(*job.mRecastMesh, job.mChangedTile, job.mAgentBounds, mSettings.get().mRecast);
            generatedNavMeshData = true;
        }

        if (preparedNavMeshData == nullptr)
        {
            Log(Debug::Debug) << "Null navmesh data for job " << job.mId;
            navMeshCacheItem.lock()->markAsEmpty(job.mChangedTile);
            return JobStatus::Done;
        }

        auto cachedNavMeshData = mNavMeshTilesCache.set(
            job.mAgentBounds, job.mChangedTile, *job.mRecastMesh, std::move(preparedNavMeshData));

        const auto offMeshConnections = mOffMeshConnectionsManager.get().get(job.mChangedTile);

        const PreparedNavMeshData* preparedNavMeshDataPtr
            = cachedNavMeshData ? &cachedNavMeshData.get() : preparedNavMeshData.get();
        assert(preparedNavMeshDataPtr != nullptr);

        const UpdateNavMeshStatus status
            = navMeshCacheItem.lock()->updateTile(job.mChangedTile, std::move(cachedNavMeshData),
                makeNavMeshTileData(*preparedNavMeshDataPtr, offMeshConnections, job.mAgentBounds, job.mChangedTile,
                    mSettings.get().mRecast));

        const JobStatus result = handleUpdateNavMeshStatus(status, job, navMeshCacheItem, *job.mRecastMesh);

        if (result == JobStatus::Done && job.mChangeType != ChangeType::update && mDbWorker != nullptr
            && mSettings.get().mWriteToNavMeshDb && generatedNavMeshData)
            job.mGeneratedNavMeshData = std::make_unique<PreparedNavMeshData>(*preparedNavMeshDataPtr);

        return result;
    }

    JobStatus AsyncNavMeshUpdater::handleUpdateNavMeshStatus(UpdateNavMeshStatus status, const Job& job,
        const GuardedNavMeshCacheItem& navMeshCacheItem, const RecastMesh& recastMesh)
    {
        const Version navMeshVersion = navMeshCacheItem.lockConst()->getVersion();
        mRecastMeshManager.get().reportNavMeshChange(job.mChangedTile, recastMesh.getVersion(), navMeshVersion);

        if (status == UpdateNavMeshStatus::removed || status == UpdateNavMeshStatus::lost)
        {
            const std::scoped_lock lock(mMutex);
            mPresentTiles.erase(std::make_tuple(job.mAgentBounds, job.mChangedTile));
        }
        else if (isSuccess(status) && status != UpdateNavMeshStatus::ignored)
        {
            const std::scoped_lock lock(mMutex);
            mPresentTiles.insert(std::make_tuple(job.mAgentBounds, job.mChangedTile));
        }

        writeDebugFiles(job, &recastMesh);

        return isSuccess(status) ? JobStatus::Done : JobStatus::Fail;
    }

    JobIt AsyncNavMeshUpdater::getNextJob()
    {
        std::unique_lock<std::mutex> lock(mMutex);

        bool shouldStop = false;
        const auto hasJob = [&] {
            shouldStop = mShouldStop;
            return shouldStop
                || (!mWaiting.empty() && mWaiting.front()->mProcessTime <= std::chrono::steady_clock::now());
        };

        if (!mHasJob.wait_for(lock, std::chrono::milliseconds(10), hasJob))
        {
            if (mJobs.empty())
                mDone.notify_all();
            return mJobs.end();
        }

        if (shouldStop)
            return mJobs.end();

        const JobIt job = mWaiting.front();

        mWaiting.pop_front();

        if (job->mRecastMesh != nullptr)
            return job;

        if (!lockTile(job->mAgentBounds, job->mChangedTile))
        {
            Log(Debug::Debug) << "Failed to lock tile by " << job->mId;
            ++job->mTryNumber;
            insertPrioritizedJob(job, mWaiting);
            return mJobs.end();
        }

        if (job->mChangeType == ChangeType::update)
            mLastUpdates[getAgentAndTile(*job)] = std::chrono::steady_clock::now();
        mPushed.erase(getAgentAndTile(*job));

        return job;
    }

    void AsyncNavMeshUpdater::writeDebugFiles(const Job& job, const RecastMesh* recastMesh) const
    {
        std::string revision;
        std::string recastMeshRevision;
        std::string navMeshRevision;
        if ((mSettings.get().mEnableWriteNavMeshToFile || mSettings.get().mEnableWriteRecastMeshToFile)
            && (mSettings.get().mEnableRecastMeshFileNameRevision || mSettings.get().mEnableNavMeshFileNameRevision))
        {
            revision = "."
                + std::to_string((std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point()).count());
            if (mSettings.get().mEnableRecastMeshFileNameRevision)
                recastMeshRevision = revision;
            if (mSettings.get().mEnableNavMeshFileNameRevision)
                navMeshRevision = revision;
        }
        if (recastMesh && mSettings.get().mEnableWriteRecastMeshToFile)
            writeToFile(*recastMesh,
                mSettings.get().mRecastMeshPathPrefix + std::to_string(job.mChangedTile.x()) + "_"
                    + std::to_string(job.mChangedTile.y()) + "_",
                recastMeshRevision, mSettings.get().mRecast);
        if (mSettings.get().mEnableWriteNavMeshToFile)
            if (const auto shared = job.mNavMeshCacheItem.lock())
                writeToFile(shared->lockConst()->getImpl(), mSettings.get().mNavMeshPathPrefix, navMeshRevision);
    }

    void AsyncNavMeshUpdater::repost(JobIt job)
    {
        unlockTile(job->mAgentBounds, job->mChangedTile);

        if (mShouldStop || job->mTryNumber > 2)
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

        if (mPushed.emplace(job->mAgentBounds, job->mChangedTile).second)
        {
            ++job->mTryNumber;
            insertPrioritizedJob(job, mWaiting);
            mHasJob.notify_all();
            return;
        }

        mJobs.erase(job);
    }

    bool AsyncNavMeshUpdater::lockTile(const AgentBounds& agentBounds, const TilePosition& changedTile)
    {
        Log(Debug::Debug) << "Locking tile agent=" << agentBounds << " changedTile=(" << changedTile << ")";
        return mProcessingTiles.lock()->emplace(agentBounds, changedTile).second;
    }

    void AsyncNavMeshUpdater::unlockTile(const AgentBounds& agentBounds, const TilePosition& changedTile)
    {
        auto locked = mProcessingTiles.lock();
        locked->erase(std::tie(agentBounds, changedTile));
        Log(Debug::Debug) << "Unlocked tile agent=" << agentBounds << " changedTile=(" << changedTile << ")";
        if (locked->empty())
            mProcessed.notify_all();
    }

    std::size_t AsyncNavMeshUpdater::getTotalJobs() const
    {
        const std::scoped_lock lock(mMutex);
        return mJobs.size();
    }

    void AsyncNavMeshUpdater::cleanupLastUpdates()
    {
        const auto now = std::chrono::steady_clock::now();

        const std::lock_guard<std::mutex> lock(mMutex);

        for (auto it = mLastUpdates.begin(); it != mLastUpdates.end();)
        {
            if (now - it->second > mSettings.get().mMinUpdateInterval)
                it = mLastUpdates.erase(it);
            else
                ++it;
        }
    }

    void AsyncNavMeshUpdater::enqueueJob(JobIt job)
    {
        Log(Debug::Debug) << "Enqueueing job " << job->mId << " by thread=" << std::this_thread::get_id();
        const std::lock_guard lock(mMutex);
        insertPrioritizedJob(job, mWaiting);
        mHasJob.notify_all();
    }

    void AsyncNavMeshUpdater::removeJob(JobIt job)
    {
        Log(Debug::Debug) << "Removing job " << job->mId << " by thread=" << std::this_thread::get_id();
        const std::lock_guard lock(mMutex);
        mJobs.erase(job);
    }

    void DbJobQueue::push(JobIt job)
    {
        const std::lock_guard lock(mMutex);
        insertPrioritizedDbJob(job, mJobs);
        if (isWritingDbJob(*job))
            ++mWritingJobs;
        else
            ++mReadingJobs;
        mHasJob.notify_all();
    }

    std::optional<JobIt> DbJobQueue::pop()
    {
        std::unique_lock lock(mMutex);
        mHasJob.wait(lock, [&] { return mShouldStop || !mJobs.empty(); });
        if (mJobs.empty())
            return std::nullopt;
        const JobIt job = mJobs.front();
        mJobs.pop_front();
        if (isWritingDbJob(*job))
            --mWritingJobs;
        else
            --mReadingJobs;
        return job;
    }

    void DbJobQueue::update(TilePosition playerTile, int maxTiles)
    {
        const std::lock_guard lock(mMutex);
        updateJobs(mJobs, playerTile, maxTiles);
        std::sort(mJobs.begin(), mJobs.end(), LessByJobDbPriority{});
    }

    void DbJobQueue::stop()
    {
        const std::lock_guard lock(mMutex);
        mJobs.clear();
        mShouldStop = true;
        mHasJob.notify_all();
    }

    DbJobQueueStats DbJobQueue::getStats() const
    {
        const std::lock_guard lock(mMutex);
        return DbJobQueueStats{ .mWritingJobs = mWritingJobs, .mReadingJobs = mReadingJobs };
    }

    DbWorker::DbWorker(AsyncNavMeshUpdater& updater, std::unique_ptr<NavMeshDb>&& db, TileVersion version,
        const RecastSettings& recastSettings, bool writeToDb)
        : mUpdater(updater)
        , mRecastSettings(recastSettings)
        , mDb(std::move(db))
        , mVersion(version)
        , mWriteToDb(writeToDb)
        , mNextTileId(mDb->getMaxTileId() + 1)
        , mNextShapeId(mDb->getMaxShapeId() + 1)
        , mThread([this] { run(); })
    {
    }

    DbWorker::~DbWorker()
    {
        stop();
    }

    void DbWorker::enqueueJob(JobIt job)
    {
        Log(Debug::Debug) << "Enqueueing db job " << job->mId << " by thread=" << std::this_thread::get_id();
        mQueue.push(job);
    }

    DbWorkerStats DbWorker::getStats() const
    {
        return DbWorkerStats{ .mJobs = mQueue.getStats(),
            .mGetTileCount = mGetTileCount.load(std::memory_order_relaxed) };
    }

    void DbWorker::stop()
    {
        mShouldStop = true;
        mQueue.stop();
        if (mThread.joinable())
            mThread.join();
    }

    void DbWorker::run() noexcept
    {
        while (!mShouldStop)
        {
            try
            {
                if (const auto job = mQueue.pop())
                    processJob(*job);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "DbWorker exception: " << e.what();
            }
        }
    }

    void DbWorker::processJob(JobIt job)
    {
        const auto process = [&](auto f) {
            try
            {
                f(job);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "DbWorker exception while processing job " << job->mId << ": " << e.what();
                if (mWriteToDb)
                {
                    const std::string_view message(e.what());
                    if (message.find("database or disk is full") != std::string_view::npos)
                    {
                        mWriteToDb = false;
                        Log(Debug::Warning)
                            << "Writes to navmeshdb are disabled because file size limit is reached or disk is full";
                    }
                    else if (message.find("database is locked") != std::string_view::npos)
                    {
                        mWriteToDb = false;
                        Log(Debug::Warning)
                            << "Writes to navmeshdb are disabled to avoid concurrent writes from multiple processes";
                    }
                }
            }
        };

        if (isWritingDbJob(*job))
        {
            process([&](JobIt job) { processWritingJob(job); });
            mUpdater.removeJob(job);
            return;
        }

        process([&](JobIt job) { processReadingJob(job); });
        job->mState = JobState::WithDbResult;
        mUpdater.enqueueJob(job);
    }

    void DbWorker::processReadingJob(JobIt job)
    {
        ++mGetTileCount;

        Log(Debug::Debug) << "Processing db read job " << job->mId;

        if (job->mInput.empty())
        {
            Log(Debug::Debug) << "Serializing input for job " << job->mId;
            if (mWriteToDb)
            {
                const auto objects = makeDbRefGeometryObjects(job->mRecastMesh->getMeshSources(),
                    [&](const MeshSource& v) { return resolveMeshSource(*mDb, v, mNextShapeId); });
                job->mInput = serialize(mRecastSettings, job->mAgentBounds, *job->mRecastMesh, objects);
            }
            else
            {
                struct HandleResult
                {
                    const RecastSettings& mRecastSettings;
                    Job& mJob;

                    bool operator()(const std::vector<DbRefGeometryObject>& objects) const
                    {
                        mJob.mInput = serialize(mRecastSettings, mJob.mAgentBounds, *mJob.mRecastMesh, objects);
                        return true;
                    }

                    bool operator()(const MeshSource& meshSource) const
                    {
                        Log(Debug::Debug) << "No object for mesh source (fileName=\"" << meshSource.mShape->mFileName
                                          << "\", areaType=" << meshSource.mAreaType
                                          << ", fileHash=" << Misc::StringUtils::toHex(meshSource.mShape->mFileHash)
                                          << ") for job " << mJob.mId;
                        return false;
                    }
                };

                const auto result = makeDbRefGeometryObjects(job->mRecastMesh->getMeshSources(),
                    [&](const MeshSource& v) { return resolveMeshSource(*mDb, v); });
                if (!std::visit(HandleResult{ mRecastSettings, *job }, result))
                    return;
            }
        }

        job->mCachedTileData = mDb->getTileData(job->mWorldspace, job->mChangedTile, job->mInput);
    }

    void DbWorker::processWritingJob(JobIt job)
    {
        if (!mWriteToDb)
        {
            Log(Debug::Debug) << "Ignored db write job " << job->mId;
            return;
        }

        Log(Debug::Debug) << "Processing db write job " << job->mId;

        if (job->mInput.empty())
        {
            Log(Debug::Debug) << "Serializing input for job " << job->mId;
            const std::vector<DbRefGeometryObject> objects
                = makeDbRefGeometryObjects(job->mRecastMesh->getMeshSources(),
                    [&](const MeshSource& v) { return resolveMeshSource(*mDb, v, mNextShapeId); });
            job->mInput = serialize(mRecastSettings, job->mAgentBounds, *job->mRecastMesh, objects);
        }

        if (const auto& cachedTileData = job->mCachedTileData)
        {
            Log(Debug::Debug) << "Update db tile by job " << job->mId;
            job->mGeneratedNavMeshData->mUserId = cachedTileData->mTileId;
            mDb->updateTile(cachedTileData->mTileId, mVersion, serialize(*job->mGeneratedNavMeshData));
            return;
        }

        const auto cached = mDb->findTile(job->mWorldspace, job->mChangedTile, job->mInput);
        if (cached.has_value() && cached->mVersion == mVersion)
        {
            Log(Debug::Debug) << "Ignore existing db tile by job " << job->mId;
            return;
        }

        job->mGeneratedNavMeshData->mUserId = mNextTileId;
        Log(Debug::Debug) << "Insert db tile by job " << job->mId;
        mDb->insertTile(mNextTileId, job->mWorldspace, job->mChangedTile, mVersion, job->mInput,
            serialize(*job->mGeneratedNavMeshData));
        ++mNextTileId;
    }
}
