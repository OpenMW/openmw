#include "asyncnavmeshupdater.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "settings.hpp"
#include "version.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/thread.hpp>
#include <components/loadinglistener/loadinglistener.hpp>

#include <DetourNavMesh.h>

#include <osg/Stats>

#include <algorithm>
#include <numeric>
#include <set>

namespace
{
    using DetourNavigator::ChangeType;
    using DetourNavigator::TilePosition;
    using DetourNavigator::UpdateType;
    using DetourNavigator::ChangeType;
    using DetourNavigator::Job;
    using DetourNavigator::JobIt;

    int getManhattanDistance(const TilePosition& lhs, const TilePosition& rhs)
    {
        return std::abs(lhs.x() - rhs.x()) + std::abs(lhs.y() - rhs.y());
    }

    int getMinDistanceTo(const TilePosition& position, int maxDistance,
                         const std::set<std::tuple<osg::Vec3f, TilePosition>>& pushedTiles,
                         const std::set<std::tuple<osg::Vec3f, TilePosition>>& presentTiles)
    {
        int result = maxDistance;
        for (const auto& [halfExtents, tile] : pushedTiles)
            if (presentTiles.find(std::tie(halfExtents, tile)) == presentTiles.end())
                result = std::min(result, getManhattanDistance(position, tile));
        return result;
    }

    UpdateType getUpdateType(ChangeType changeType) noexcept
    {
        if (changeType == ChangeType::update)
            return UpdateType::Temporary;
        return UpdateType::Persistent;
    }

    auto getPriority(const Job& job) noexcept
    {
        return std::make_tuple(job.mProcessTime, job.mChangeType, job.mTryNumber, job.mDistanceToPlayer, job.mDistanceToOrigin);
    }

    struct LessByJobPriority
    {
        bool operator()(JobIt lhs, JobIt rhs) const noexcept
        {
            return getPriority(*lhs) < getPriority(*rhs);
        }
    };

    void insertPrioritizedJob(JobIt job, std::deque<JobIt>& queue)
    {
        const auto it = std::upper_bound(queue.begin(), queue.end(), job, LessByJobPriority {});
        queue.insert(it, job);
    }

    auto getAgentAndTile(const Job& job) noexcept
    {
        return std::make_tuple(job.mAgentHalfExtents, job.mChangedTile);
    }
}

namespace DetourNavigator
{
    Job::Job(const osg::Vec3f& agentHalfExtents, std::weak_ptr<GuardedNavMeshCacheItem> navMeshCacheItem,
        const TilePosition& changedTile, ChangeType changeType, int distanceToPlayer,
        std::chrono::steady_clock::time_point processTime)
        : mAgentHalfExtents(agentHalfExtents)
        , mNavMeshCacheItem(std::move(navMeshCacheItem))
        , mChangedTile(changedTile)
        , mProcessTime(processTime)
        , mChangeType(changeType)
        , mDistanceToPlayer(distanceToPlayer)
        , mDistanceToOrigin(getManhattanDistance(changedTile, TilePosition {0, 0}))
    {
    }

    AsyncNavMeshUpdater::AsyncNavMeshUpdater(const Settings& settings, TileCachedRecastMeshManager& recastMeshManager,
            OffMeshConnectionsManager& offMeshConnectionsManager)
        : mSettings(settings)
        , mRecastMeshManager(recastMeshManager)
        , mOffMeshConnectionsManager(offMeshConnectionsManager)
        , mShouldStop()
        , mNavMeshTilesCache(settings.mMaxNavMeshTilesCacheSize)
    {
        for (std::size_t i = 0; i < mSettings.get().mAsyncNavMeshUpdaterThreads; ++i)
            mThreads.emplace_back([&] { process(); });
    }

    AsyncNavMeshUpdater::~AsyncNavMeshUpdater()
    {
        mShouldStop = true;
        std::unique_lock<std::mutex> lock(mMutex);
        mWaiting.clear();
        mHasJob.notify_all();
        lock.unlock();
        for (auto& thread : mThreads)
            thread.join();
    }

    void AsyncNavMeshUpdater::post(const osg::Vec3f& agentHalfExtents,
        const SharedNavMeshCacheItem& navMeshCacheItem, const TilePosition& playerTile,
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

        const std::lock_guard<std::mutex> lock(mMutex);

        if (playerTileChanged)
        {
            for (JobIt job : mWaiting)
            {
                job->mDistanceToPlayer = getManhattanDistance(job->mChangedTile, playerTile);
                if (!shouldAddTile(job->mChangedTile, playerTile, std::min(mSettings.get().mMaxTilesNumber, params.maxTiles)))
                    job->mChangeType = ChangeType::remove;
            }
        }

        for (const auto& [changedTile, changeType] : changedTiles)
        {
            if (mPushed.emplace(agentHalfExtents, changedTile).second)
            {
                const auto processTime = changeType == ChangeType::update
                    ? mLastUpdates[std::tie(agentHalfExtents, changedTile)] + mSettings.get().mMinUpdateInterval
                    : std::chrono::steady_clock::time_point();

                const JobIt it = mJobs.emplace(mJobs.end(), agentHalfExtents, navMeshCacheItem, changedTile,
                    changeType, getManhattanDistance(changedTile, playerTile), processTime);

                if (playerTileChanged)
                    mWaiting.push_back(it);
                else
                    insertPrioritizedJob(it, mWaiting);
            }
        }

        if (playerTileChanged)
            std::sort(mWaiting.begin(), mWaiting.end(), LessByJobPriority {});

        Log(Debug::Debug) << "Posted " << mJobs.size() << " navigator jobs";

        if (!mWaiting.empty())
            mHasJob.notify_all();
    }

    void AsyncNavMeshUpdater::wait(Loading::Listener& listener, WaitConditionType waitConditionType)
    {
        if (mSettings.get().mWaitUntilMinDistanceToPlayer == 0)
            return;
        listener.setLabel("Building navigation mesh");
        const std::size_t initialJobsLeft = getTotalJobs();
        std::size_t maxProgress = initialJobsLeft + mThreads.size();
        listener.setProgressRange(maxProgress);
        switch (waitConditionType)
        {
            case WaitConditionType::requiredTilesPresent:
            {
                const int minDistanceToPlayer = waitUntilJobsDoneForNotPresentTiles(initialJobsLeft, maxProgress, listener);
                if (minDistanceToPlayer < mSettings.get().mWaitUntilMinDistanceToPlayer)
                {
                    mProcessingTiles.wait(mProcessed, [] (const auto& v) { return v.empty(); });
                    listener.setProgress(maxProgress);
                }
                break;
            }
            case WaitConditionType::allJobsDone:
                waitUntilAllJobsDone();
                listener.setProgress(maxProgress);
                break;
        }
    }

    int AsyncNavMeshUpdater::waitUntilJobsDoneForNotPresentTiles(const std::size_t initialJobsLeft, std::size_t& maxProgress, Loading::Listener& listener)
    {
        std::size_t prevJobsLeft = initialJobsLeft;
        std::size_t jobsDone = 0;
        std::size_t jobsLeft = 0;
        const int maxDistanceToPlayer = mSettings.get().mWaitUntilMinDistanceToPlayer;
        const TilePosition playerPosition = *mPlayerTile.lockConst();
        int minDistanceToPlayer = 0;
        const auto isDone = [&]
        {
            jobsLeft = mJobs.size();
            if (jobsLeft == 0)
            {
                minDistanceToPlayer = 0;
                return true;
            }
            minDistanceToPlayer = getMinDistanceTo(playerPosition, maxDistanceToPlayer, mPushed, mPresentTiles);
            return minDistanceToPlayer >= maxDistanceToPlayer;
        };
        std::unique_lock<std::mutex> lock(mMutex);
        while (!mDone.wait_for(lock, std::chrono::milliseconds(250), isDone))
        {
            if (maxProgress < jobsLeft)
            {
                maxProgress = jobsLeft + mThreads.size();
                listener.setProgressRange(maxProgress);
                listener.setProgress(jobsDone);
            }
            else if (jobsLeft < prevJobsLeft)
            {
                const std::size_t newJobsDone = prevJobsLeft - jobsLeft;
                jobsDone += newJobsDone;
                prevJobsLeft = jobsLeft;
                listener.increaseProgress(newJobsDone);
            }
        }
        return minDistanceToPlayer;
    }

    void AsyncNavMeshUpdater::waitUntilAllJobsDone()
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mDone.wait(lock, [this] { return mJobs.size() == 0; });
        }
        mProcessingTiles.wait(mProcessed, [] (const auto& v) { return v.empty(); });
    }

    void AsyncNavMeshUpdater::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        std::size_t jobs = 0;
        std::size_t waiting = 0;
        std::size_t pushed = 0;

        {
            const std::lock_guard<std::mutex> lock(mMutex);
            jobs = mJobs.size();
            waiting = mWaiting.size();
            pushed = mPushed.size();
        }

        stats.setAttribute(frameNumber, "NavMesh Jobs", jobs);
        stats.setAttribute(frameNumber, "NavMesh Waiting", waiting);
        stats.setAttribute(frameNumber, "NavMesh Pushed", pushed);
        stats.setAttribute(frameNumber, "NavMesh Processing", mProcessingTiles.lockConst()->size());

        mNavMeshTilesCache.reportStats(frameNumber, stats);
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
                    const auto processed = processJob(*job);
                    unlockTile(job->mAgentHalfExtents, job->mChangedTile);
                    if (processed)
                        removeJob(job);
                    else
                        repost(job);
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

    bool AsyncNavMeshUpdater::processJob(const Job& job)
    {
        Log(Debug::Debug) << "Process job for agent=(" << std::fixed << std::setprecision(2) << job.mAgentHalfExtents << ")"
            " by thread=" << std::this_thread::get_id();

        const auto start = std::chrono::steady_clock::now();

        const auto navMeshCacheItem = job.mNavMeshCacheItem.lock();

        if (!navMeshCacheItem)
            return true;

        const auto recastMesh = mRecastMeshManager.get().getMesh(job.mChangedTile);
        const auto playerTile = *mPlayerTile.lockConst();
        const auto offMeshConnections = mOffMeshConnectionsManager.get().get(job.mChangedTile);

        const auto status = updateNavMesh(job.mAgentHalfExtents, recastMesh.get(), job.mChangedTile, playerTile,
            offMeshConnections, mSettings, navMeshCacheItem, mNavMeshTilesCache, getUpdateType(job.mChangeType));

        if (recastMesh != nullptr)
        {
            Version navMeshVersion;
            {
                const auto locked = navMeshCacheItem->lockConst();
                navMeshVersion.mGeneration = locked->getGeneration();
                navMeshVersion.mRevision = locked->getNavMeshRevision();
            }
            mRecastMeshManager.get().reportNavMeshChange(job.mChangedTile,
                Version {recastMesh->getGeneration(), recastMesh->getRevision()},
                navMeshVersion);
        }

        if (status == UpdateNavMeshStatus::removed || status == UpdateNavMeshStatus::lost)
        {
            const std::scoped_lock lock(mMutex);
            mPresentTiles.erase(std::make_tuple(job.mAgentHalfExtents, job.mChangedTile));
        }
        else if (isSuccess(status) && status != UpdateNavMeshStatus::ignored)
        {
            const std::scoped_lock lock(mMutex);
            mPresentTiles.insert(std::make_tuple(job.mAgentHalfExtents, job.mChangedTile));
        }

        const auto finish = std::chrono::steady_clock::now();

        writeDebugFiles(job, recastMesh.get());

        using FloatMs = std::chrono::duration<float, std::milli>;

        const auto locked = navMeshCacheItem->lockConst();
        Log(Debug::Debug) << std::fixed << std::setprecision(2) <<
            "Cache updated for agent=(" << job.mAgentHalfExtents << ")" <<
            " tile=" << job.mChangedTile <<
            " status=" << status <<
            " generation=" << locked->getGeneration() <<
            " revision=" << locked->getNavMeshRevision() <<
            " time=" << std::chrono::duration_cast<FloatMs>(finish - start).count() << "ms" <<
            " thread=" << std::this_thread::get_id();

        return isSuccess(status);
    }

    JobIt AsyncNavMeshUpdater::getNextJob()
    {
        std::unique_lock<std::mutex> lock(mMutex);

        bool shouldStop = false;
        const auto hasJob = [&]
        {
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

        if (!lockTile(job->mAgentHalfExtents, job->mChangedTile))
        {
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
            revision = "." + std::to_string((std::chrono::steady_clock::now()
                - std::chrono::steady_clock::time_point()).count());
            if (mSettings.get().mEnableRecastMeshFileNameRevision)
                recastMeshRevision = revision;
            if (mSettings.get().mEnableNavMeshFileNameRevision)
                navMeshRevision = revision;
        }
        if (recastMesh && mSettings.get().mEnableWriteRecastMeshToFile)
            writeToFile(*recastMesh, mSettings.get().mRecastMeshPathPrefix + std::to_string(job.mChangedTile.x())
                        + "_" + std::to_string(job.mChangedTile.y()) + "_", recastMeshRevision);
        if (mSettings.get().mEnableWriteNavMeshToFile)
            if (const auto shared = job.mNavMeshCacheItem.lock())
                writeToFile(shared->lockConst()->getImpl(), mSettings.get().mNavMeshPathPrefix, navMeshRevision);
    }

    void AsyncNavMeshUpdater::repost(JobIt job)
    {
        if (mShouldStop || job->mTryNumber > 2)
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

        if (mPushed.emplace(job->mAgentHalfExtents, job->mChangedTile).second)
        {
            ++job->mTryNumber;
            insertPrioritizedJob(job, mWaiting);
            mHasJob.notify_all();
            return;
        }

        mJobs.erase(job);
    }

    bool AsyncNavMeshUpdater::lockTile(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile)
    {
        if (mSettings.get().mAsyncNavMeshUpdaterThreads <= 1)
            return true;
        return mProcessingTiles.lock()->emplace(agentHalfExtents, changedTile).second;
    }

    void AsyncNavMeshUpdater::unlockTile(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile)
    {
        if (mSettings.get().mAsyncNavMeshUpdaterThreads <= 1)
            return;
        auto locked = mProcessingTiles.lock();
        locked->erase(std::tie(agentHalfExtents, changedTile));
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

    void AsyncNavMeshUpdater::removeJob(JobIt job)
    {
        const std::lock_guard lock(mMutex);
        mJobs.erase(job);
    }
}
