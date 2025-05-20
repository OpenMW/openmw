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
#include <components/misc/strings/format.hpp>
#include <components/misc/thread.hpp>

#include <DetourNavMesh.h>

#include <osg/io_utils>

#include <boost/geometry.hpp>

#include <algorithm>
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

        std::size_t getNextJobId()
        {
            static std::atomic_size_t nextJobId{ 1 };
            return nextJobId.fetch_add(1);
        }

        bool isWritingDbJob(const Job& job)
        {
            return job.mGeneratedNavMeshData != nullptr;
        }

        std::string makeRevision(const Version& version)
        {
            return Misc::StringUtils::format(".%zu.%zu", version.mGeneration, version.mRevision);
        }

        void writeDebugRecastMesh(
            const Settings& settings, const TilePosition& tilePosition, const RecastMesh& recastMesh)
        {
            if (!settings.mEnableWriteRecastMeshToFile)
                return;
            std::string revision;
            if (settings.mEnableRecastMeshFileNameRevision)
                revision = makeRevision(recastMesh.getVersion());
            writeToFile(recastMesh,
                Misc::StringUtils::format(
                    "%s%d.%d.", settings.mRecastMeshPathPrefix, tilePosition.x(), tilePosition.y()),
                revision, settings.mRecast);
        }

        void writeDebugNavMesh(
            const Settings& settings, const GuardedNavMeshCacheItem& navMeshCacheItem, const Version& version)
        {
            if (!settings.mEnableWriteNavMeshToFile)
                return;
            std::string revision;
            if (settings.mEnableNavMeshFileNameRevision)
                revision = makeRevision(version);
            writeToFile(navMeshCacheItem.lockConst()->getImpl(), settings.mNavMeshPathPrefix, revision);
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
        ESM::RefId worldspace, const TilePosition& changedTile, ChangeType changeType,
        std::chrono::steady_clock::time_point processTime)
        : mId(getNextJobId())
        , mAgentBounds(agentBounds)
        , mNavMeshCacheItem(std::move(navMeshCacheItem))
        , mWorldspace(worldspace)
        , mChangedTile(changedTile)
        , mProcessTime(processTime)
        , mChangeType(changeType)
    {
    }

    void SpatialJobQueue::clear()
    {
        mValues.clear();
        mIndex.clear();
        mSize = 0;
    }

    void SpatialJobQueue::push(JobIt job)
    {
        auto it = mValues.find(job->mChangedTile);

        if (it == mValues.end())
        {
            it = mValues.emplace_hint(it, job->mChangedTile, std::deque<JobIt>());
            mIndex.insert(IndexValue(IndexPoint(job->mChangedTile.x(), job->mChangedTile.y()), it));
        }

        it->second.push_back(job);

        ++mSize;
    }

    std::optional<JobIt> SpatialJobQueue::pop(TilePosition playerTile)
    {
        const IndexPoint point(playerTile.x(), playerTile.y());
        const auto it = mIndex.qbegin(boost::geometry::index::nearest(point, 1));

        if (it == mIndex.qend())
            return std::nullopt;

        const UpdatingMap::iterator mapIt = it->second;
        std::deque<JobIt>& tileJobs = mapIt->second;
        JobIt result = tileJobs.front();
        tileJobs.pop_front();

        --mSize;

        if (tileJobs.empty())
        {
            mValues.erase(mapIt);
            mIndex.remove(*it);
        }

        return result;
    }

    void SpatialJobQueue::update(TilePosition playerTile, int maxTiles, std::vector<JobIt>& removing)
    {
        for (auto it = mValues.begin(); it != mValues.end();)
        {
            if (shouldAddTile(it->first, playerTile, maxTiles))
            {
                ++it;
                continue;
            }

            for (JobIt job : it->second)
            {
                job->mChangeType = ChangeType::remove;
                removing.push_back(job);
            }

            mSize -= it->second.size();
            mIndex.remove(IndexValue(IndexPoint(it->first.x(), it->first.y()), it));
            it = mValues.erase(it);
        }
    }

    bool JobQueue::hasJob(std::chrono::steady_clock::time_point now) const
    {
        return !mRemoving.empty() || mUpdating.size() > 0
            || (!mDelayed.empty() && mDelayed.front()->mProcessTime <= now);
    }

    void JobQueue::clear()
    {
        mRemoving.clear();
        mDelayed.clear();
        mUpdating.clear();
    }

    void JobQueue::push(JobIt job, std::chrono::steady_clock::time_point now)
    {
        if (job->mProcessTime > now)
        {
            mDelayed.push_back(job);
            return;
        }

        if (job->mChangeType == ChangeType::remove)
        {
            mRemoving.push_back(job);
            return;
        }

        mUpdating.push(job);
    }

    std::optional<JobIt> JobQueue::pop(TilePosition playerTile, std::chrono::steady_clock::time_point now)
    {
        if (!mRemoving.empty())
        {
            const JobIt result = mRemoving.back();
            mRemoving.pop_back();
            return result;
        }

        if (const std::optional<JobIt> result = mUpdating.pop(playerTile))
            return result;

        if (mDelayed.empty() || mDelayed.front()->mProcessTime > now)
            return std::nullopt;

        const JobIt result = mDelayed.front();
        mDelayed.pop_front();
        return result;
    }

    void JobQueue::update(TilePosition playerTile, int maxTiles, std::chrono::steady_clock::time_point now)
    {
        mUpdating.update(playerTile, maxTiles, mRemoving);

        while (!mDelayed.empty() && mDelayed.front()->mProcessTime <= now)
        {
            const JobIt job = mDelayed.front();
            mDelayed.pop_front();

            if (shouldAddTile(job->mChangedTile, playerTile, maxTiles))
            {
                mUpdating.push(job);
            }
            else
            {
                job->mChangeType = ChangeType::remove;
                mRemoving.push_back(job);
            }
        }
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
        const TilePosition& playerTile, ESM::RefId worldspace, const std::map<TilePosition, ChangeType>& changedTiles)
    {
        bool playerTileChanged = false;
        {
            auto locked = mPlayerTile.lock();
            playerTileChanged = *locked != playerTile;
            *locked = playerTile;
        }

        if (!playerTileChanged && changedTiles.empty())
            return;

        std::unique_lock lock(mMutex);

        if (playerTileChanged)
        {
            Log(Debug::Debug) << "Player tile has been changed to " << playerTile;
            mWaiting.update(playerTile, mSettings.get().mMaxTilesNumber);
        }

        for (const auto& [changedTile, changeType] : changedTiles)
        {
            if (mPushed.emplace(agentBounds, changedTile).second)
            {
                const auto processTime = [&, changedTile = changedTile, changeType = changeType] {
                    if (changeType != ChangeType::update)
                        return std::chrono::steady_clock::time_point();
                    const auto lastUpdate = mLastUpdates.find(std::tie(agentBounds, changedTile));
                    if (lastUpdate == mLastUpdates.end())
                        return std::chrono::steady_clock::time_point();
                    return lastUpdate->second + mSettings.get().mMinUpdateInterval;
                }();

                const JobIt it = mJobs.emplace(
                    mJobs.end(), agentBounds, navMeshCacheItem, worldspace, changedTile, changeType, processTime);

                Log(Debug::Debug) << "Post job " << it->mId << " for agent=(" << it->mAgentBounds << ")"
                                  << " changedTile=(" << it->mChangedTile << ")"
                                  << " changeType=" << it->mChangeType;

                mWaiting.push(it);
            }
        }

        Log(Debug::Debug) << "Posted " << mJobs.size() << " navigator jobs";

        if (mWaiting.hasJob())
            mHasJob.notify_all();

        lock.unlock();

        if (playerTileChanged && mDbWorker != nullptr)
            mDbWorker->update(playerTile);
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
            result.mWaiting = mWaiting.getStats();
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
            if (JobIt job = getNextJob(); job != mJobs.end())
            {
                try
                {
                    const JobStatus status = processJob(*job);
                    Log(Debug::Debug) << "Processed job " << job->mId << " with status=" << status
                                      << " changeType=" << job->mChangeType;
                    switch (status)
                    {
                        case JobStatus::Done:
                            unlockTile(job->mId, job->mAgentBounds, job->mChangedTile);
                            if (job->mGeneratedNavMeshData != nullptr)
                                mDbWorker->enqueueJob(job);
                            else
                                removeJob(job);
                            break;
                        case JobStatus::Fail:
                            unlockTile(job->mId, job->mAgentBounds, job->mChangedTile);
                            removeJob(job);
                            break;
                        case JobStatus::MemoryCacheMiss:
                        {
                            mDbWorker->enqueueJob(job);
                            break;
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    Log(Debug::Warning) << "Failed to process navmesh job " << job->mId
                                        << " for worldspace=" << job->mWorldspace << " agent=" << job->mAgentBounds
                                        << " changedTile=(" << job->mChangedTile << ")"
                                        << " changeType=" << job->mChangeType
                                        << " by thread=" << std::this_thread::get_id() << ": " << e.what();
                    unlockTile(job->mId, job->mAgentBounds, job->mChangedTile);
                    removeJob(job);
                }
            }
            else
            {
                cleanupLastUpdates();
            }
        }
        Log(Debug::Debug) << "Stop navigator jobs processing by thread=" << std::this_thread::get_id();
    }

    JobStatus AsyncNavMeshUpdater::processJob(Job& job)
    {
        Log(Debug::Debug) << "Processing job " << job.mId << "  for worldspace=" << job.mWorldspace
                          << " agent=" << job.mAgentBounds << ""
                          << " changedTile=(" << job.mChangedTile << ")"
                          << " changeType=" << job.mChangeType << " by thread=" << std::this_thread::get_id();

        const auto navMeshCacheItem = job.mNavMeshCacheItem.lock();

        if (!navMeshCacheItem)
            return JobStatus::Done;

        const auto playerTile = *mPlayerTile.lockConst();

        if (!shouldAddTile(job.mChangedTile, playerTile, mSettings.get().mMaxTilesNumber))
        {
            Log(Debug::Debug) << "Ignore add tile by job " << job.mId << ": too far from player";
            job.mChangeType = ChangeType::remove;
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

        try
        {
            writeDebugRecastMesh(mSettings, job.mChangedTile, *recastMesh);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Warning) << "Failed to write debug recast mesh: " << e.what();
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

            preparedNavMeshData = prepareNavMeshTileData(
                *recastMesh, job.mWorldspace, job.mChangedTile, job.mAgentBounds, mSettings.get().mRecast);

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

        assert(preparedNavMeshDataPtr != nullptr);

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
            preparedNavMeshData = prepareNavMeshTileData(
                *job.mRecastMesh, job.mWorldspace, job.mChangedTile, job.mAgentBounds, mSettings.get().mRecast);
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

        try
        {
            writeDebugNavMesh(mSettings, navMeshCacheItem, navMeshVersion);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Warning) << "Failed to write debug navmesh: " << e.what();
        }

        return isSuccess(status) ? JobStatus::Done : JobStatus::Fail;
    }

    JobIt AsyncNavMeshUpdater::getNextJob() noexcept
    {
        std::unique_lock<std::mutex> lock(mMutex);

        bool shouldStop = false;
        const auto hasJob = [&] {
            shouldStop = mShouldStop.load();
            return shouldStop || mWaiting.hasJob();
        };

        if (!mHasJob.wait_for(lock, std::chrono::milliseconds(10), hasJob))
        {
            if (mJobs.empty())
                mDone.notify_all();
            return mJobs.end();
        }

        if (shouldStop)
            return mJobs.end();

        const TilePosition playerTile = *mPlayerTile.lockConst();

        JobIt job = mJobs.end();

        if (const std::optional<JobIt> nextJob = mWaiting.pop(playerTile))
            job = *nextJob;

        if (job == mJobs.end())
            return job;

        Log(Debug::Debug) << "Pop job " << job->mId << " by thread=" << std::this_thread::get_id();

        if (job->mRecastMesh != nullptr)
            return job;

        if (!lockTile(job->mId, job->mAgentBounds, job->mChangedTile))
        {
            Log(Debug::Debug) << "Failed to lock tile by job " << job->mId;
            job->mProcessTime = std::chrono::steady_clock::now() + mSettings.get().mMinUpdateInterval;
            mWaiting.push(job);
            return mJobs.end();
        }

        if (job->mChangeType == ChangeType::update)
            mLastUpdates[getAgentAndTile(*job)] = std::chrono::steady_clock::now();
        mPushed.erase(getAgentAndTile(*job));

        return job;
    }

    bool AsyncNavMeshUpdater::lockTile(
        std::size_t jobId, const AgentBounds& agentBounds, const TilePosition& changedTile)
    {
        Log(Debug::Debug) << "Locking tile by job " << jobId << " agent=" << agentBounds << " changedTile=("
                          << changedTile << ")";
        return mProcessingTiles.lock()->emplace(agentBounds, changedTile).second;
    }

    void AsyncNavMeshUpdater::unlockTile(
        std::size_t jobId, const AgentBounds& agentBounds, const TilePosition& changedTile)
    {
        auto locked = mProcessingTiles.lock();
        locked->erase(std::tie(agentBounds, changedTile));
        Log(Debug::Debug) << "Unlocked tile by job " << jobId << " agent=" << agentBounds << " changedTile=("
                          << changedTile << ")";
        if (locked->empty())
            mProcessed.notify_all();
    }

    std::size_t AsyncNavMeshUpdater::getTotalJobs() const
    {
        const std::scoped_lock lock(mMutex);
        return mJobs.size();
    }

    void AsyncNavMeshUpdater::cleanupLastUpdates() noexcept
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
        mWaiting.push(job);
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
        if (isWritingDbJob(*job))
            mWriting.push_back(job);
        else
            mReading.push(job);
        mHasJob.notify_all();
    }

    std::optional<JobIt> DbJobQueue::pop()
    {
        std::unique_lock lock(mMutex);

        const auto hasJob = [&] { return mShouldStop || mReading.size() > 0 || mWriting.size() > 0; };

        mHasJob.wait(lock, hasJob);

        if (mShouldStop)
            return std::nullopt;

        if (const std::optional<JobIt> job = mReading.pop(mPlayerTile))
            return job;

        if (mWriting.empty())
            return std::nullopt;

        const JobIt job = mWriting.front();
        mWriting.pop_front();

        return job;
    }

    void DbJobQueue::update(TilePosition playerTile)
    {
        const std::lock_guard lock(mMutex);
        mPlayerTile = playerTile;
    }

    void DbJobQueue::stop()
    {
        const std::lock_guard lock(mMutex);
        mReading.clear();
        mWriting.clear();
        mShouldStop = true;
        mHasJob.notify_all();
    }

    DbJobQueueStats DbJobQueue::getStats() const
    {
        const std::lock_guard lock(mMutex);
        return DbJobQueueStats{
            .mReadingJobs = mReading.size(),
            .mWritingJobs = mWriting.size(),
        };
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
        return DbWorkerStats{
            .mJobs = mQueue.getStats(),
            .mGetTileCount = mGetTileCount.load(std::memory_order_relaxed),
        };
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
                    else if (message.find("UNIQUE constraint failed: tiles.tile_id") != std::string_view::npos)
                    {
                        Log(Debug::Warning) << "Found duplicate navmeshdb tile_id, please report the "
                                               "issue to https://gitlab.com/OpenMW/openmw/-/issues, attach openmw.log: "
                                            << mNextTileId;
                        try
                        {
                            mNextTileId = TileId(mDb->getMaxTileId() + 1);
                            Log(Debug::Info) << "Updated navmeshdb tile_id to: " << mNextTileId;
                        }
                        catch (const std::exception& e)
                        {
                            mWriteToDb = false;
                            Log(Debug::Warning)
                                << "Failed to update next tile_id, writes to navmeshdb are disabled: " << e.what();
                        }
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
