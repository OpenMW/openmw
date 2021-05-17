#include "asyncnavmeshupdater.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "settings.hpp"
#include "version.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/thread.hpp>
#include <components/loadinglistener/loadinglistener.hpp>

#include <osg/Stats>

#include <algorithm>
#include <numeric>
#include <set>

namespace
{
    using DetourNavigator::ChangeType;
    using DetourNavigator::TilePosition;

    int getManhattanDistance(const TilePosition& lhs, const TilePosition& rhs)
    {
        return std::abs(lhs.x() - rhs.x()) + std::abs(lhs.y() - rhs.y());
    }

    int getMinDistanceTo(const TilePosition& position, int maxDistance,
                         const std::map<osg::Vec3f, std::set<TilePosition>>& tilesPerHalfExtents,
                         const std::set<std::tuple<osg::Vec3f, TilePosition>>& presentTiles)
    {
        int result = maxDistance;
        for (const auto& [halfExtents, tiles] : tilesPerHalfExtents)
            for (const TilePosition& tile : tiles)
                if (presentTiles.find(std::make_tuple(halfExtents, tile)) == presentTiles.end())
                    result = std::min(result, getManhattanDistance(position, tile));
        return result;
    }
}

namespace DetourNavigator
{
    static std::ostream& operator <<(std::ostream& stream, UpdateNavMeshStatus value)
    {
        switch (value)
        {
            case UpdateNavMeshStatus::ignored:
                return stream << "ignore";
            case UpdateNavMeshStatus::removed:
                return stream << "removed";
            case UpdateNavMeshStatus::added:
                return stream << "add";
            case UpdateNavMeshStatus::replaced:
                return stream << "replaced";
            case UpdateNavMeshStatus::failed:
                return stream << "failed";
            case UpdateNavMeshStatus::lost:
                return stream << "lost";
            case UpdateNavMeshStatus::cached:
                return stream << "cached";
            case UpdateNavMeshStatus::unchanged:
                return stream << "unchanged";
            case UpdateNavMeshStatus::restored:
                return stream << "restored";
        }
        return stream << "unknown(" << static_cast<unsigned>(value) << ")";
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
        mJobs = decltype(mJobs)();
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

        const std::lock_guard<std::mutex> lock(mMutex);

        if (playerTileChanged)
            for (auto& job : mJobs)
                job.mDistanceToPlayer = getManhattanDistance(job.mChangedTile, playerTile);

        for (const auto& changedTile : changedTiles)
        {
            if (mPushed[agentHalfExtents].insert(changedTile.first).second)
            {
                Job job;

                job.mAgentHalfExtents = agentHalfExtents;
                job.mNavMeshCacheItem = navMeshCacheItem;
                job.mChangedTile = changedTile.first;
                job.mTryNumber = 0;
                job.mChangeType = changedTile.second;
                job.mDistanceToPlayer = getManhattanDistance(changedTile.first, playerTile);
                job.mDistanceToOrigin = getManhattanDistance(changedTile.first, TilePosition {0, 0});
                job.mProcessTime = job.mChangeType == ChangeType::update
                    ? mLastUpdates[job.mAgentHalfExtents][job.mChangedTile] + mSettings.get().mMinUpdateInterval
                    : std::chrono::steady_clock::time_point();

                if (playerTileChanged)
                {
                    mJobs.push_back(std::move(job));
                }
                else
                {
                    const auto it = std::upper_bound(mJobs.begin(), mJobs.end(), job);
                    mJobs.insert(it, std::move(job));
                }
            }
        }

        if (playerTileChanged)
            std::sort(mJobs.begin(), mJobs.end());

        Log(Debug::Debug) << "Posted " << mJobs.size() << " navigator jobs";

        if (!mJobs.empty())
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
            jobsLeft = mJobs.size() + getTotalThreadJobsUnsafe();
            if (jobsLeft == 0)
            {
                minDistanceToPlayer = 0;
                return true;
            }
            minDistanceToPlayer = getMinDistanceTo(playerPosition, maxDistanceToPlayer, mPushed, mPresentTiles);
            for (const auto& [threadId, queue] : mThreadsQueues)
                minDistanceToPlayer = getMinDistanceTo(playerPosition, minDistanceToPlayer, queue.mPushed, mPresentTiles);
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
            mDone.wait(lock, [this] { return mJobs.size() + getTotalThreadJobsUnsafe() == 0; });
        }
        mProcessingTiles.wait(mProcessed, [] (const auto& v) { return v.empty(); });
    }

    void AsyncNavMeshUpdater::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        std::size_t jobs = 0;

        {
            const std::lock_guard<std::mutex> lock(mMutex);
            jobs = mJobs.size() + getTotalThreadJobsUnsafe();
        }

        stats.setAttribute(frameNumber, "NavMesh UpdateJobs", jobs);

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
                if (auto job = getNextJob())
                {
                    const auto processed = processJob(*job);
                    unlockTile(job->mAgentHalfExtents, job->mChangedTile);
                    if (!processed)
                        repost(std::move(*job));
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

        const auto firstStart = setFirstStart(start);

        const auto navMeshCacheItem = job.mNavMeshCacheItem.lock();

        if (!navMeshCacheItem)
            return true;

        const auto recastMesh = mRecastMeshManager.get().getMesh(job.mChangedTile);
        const auto playerTile = *mPlayerTile.lockConst();
        const auto offMeshConnections = mOffMeshConnectionsManager.get().get(job.mChangedTile);

        const auto status = updateNavMesh(job.mAgentHalfExtents, recastMesh.get(), job.mChangedTile, playerTile,
            offMeshConnections, mSettings, navMeshCacheItem, mNavMeshTilesCache);

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
            " total_time=" << std::chrono::duration_cast<FloatMs>(finish - firstStart).count() << "ms"
            " thread=" << std::this_thread::get_id();

        return isSuccess(status);
    }

    std::optional<AsyncNavMeshUpdater::Job> AsyncNavMeshUpdater::getNextJob()
    {
        std::unique_lock<std::mutex> lock(mMutex);

        const auto threadId = std::this_thread::get_id();
        auto& threadQueue = mThreadsQueues[threadId];

        while (true)
        {
            const auto hasJob = [&] {
                return (!mJobs.empty() && mJobs.front().mProcessTime <= std::chrono::steady_clock::now())
                    || !threadQueue.mJobs.empty();
            };

            if (!mHasJob.wait_for(lock, std::chrono::milliseconds(10), hasJob))
            {
                mFirstStart.lock()->reset();
                if (mJobs.empty() && getTotalThreadJobsUnsafe() == 0)
                    mDone.notify_all();
                return std::nullopt;
            }

            Log(Debug::Debug) << "Got " << mJobs.size() << " navigator jobs and "
                << threadQueue.mJobs.size() << " thread jobs by thread=" << std::this_thread::get_id();

            auto job = threadQueue.mJobs.empty()
                ? getJob(mJobs, mPushed, true)
                : getJob(threadQueue.mJobs, threadQueue.mPushed, false);

            if (!job)
                continue;

            const auto owner = lockTile(job->mAgentHalfExtents, job->mChangedTile);

            if (owner == threadId)
                return job;

            postThreadJob(std::move(*job), mThreadsQueues[owner]);
        }
    }

    std::optional<AsyncNavMeshUpdater::Job> AsyncNavMeshUpdater::getJob(Jobs& jobs, Pushed& pushed, bool changeLastUpdate)
    {
        const auto now = std::chrono::steady_clock::now();

        if (jobs.front().mProcessTime > now)
            return {};

        Job job = jobs.front();
        jobs.pop_front();

        if (changeLastUpdate && job.mChangeType == ChangeType::update)
            mLastUpdates[job.mAgentHalfExtents][job.mChangedTile] = now;

        const auto it = pushed.find(job.mAgentHalfExtents);
        it->second.erase(job.mChangedTile);
        if (it->second.empty())
            pushed.erase(it);

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

    std::chrono::steady_clock::time_point AsyncNavMeshUpdater::setFirstStart(const std::chrono::steady_clock::time_point& value)
    {
        const auto locked = mFirstStart.lock();
        if (!*locked)
            *locked = value;
        return *locked.get();
    }

    void AsyncNavMeshUpdater::repost(Job&& job)
    {
        if (mShouldStop || job.mTryNumber > 2)
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

        if (mPushed[job.mAgentHalfExtents].insert(job.mChangedTile).second)
        {
            ++job.mTryNumber;
            mJobs.push_back(std::move(job));
            mHasJob.notify_all();
        }
    }

    void AsyncNavMeshUpdater::postThreadJob(Job&& job, Queue& queue)
    {
        if (queue.mPushed[job.mAgentHalfExtents].insert(job.mChangedTile).second)
        {
            queue.mJobs.push_back(std::move(job));
            mHasJob.notify_all();
        }
    }

    std::thread::id AsyncNavMeshUpdater::lockTile(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile)
    {
        if (mSettings.get().mAsyncNavMeshUpdaterThreads <= 1)
            return std::this_thread::get_id();

        auto locked = mProcessingTiles.lock();

        auto agent = locked->find(agentHalfExtents);
        if (agent == locked->end())
        {
            const auto threadId = std::this_thread::get_id();
            locked->emplace(agentHalfExtents, std::map<TilePosition, std::thread::id>({{changedTile, threadId}}));
            return threadId;
        }

        auto tile = agent->second.find(changedTile);
        if (tile == agent->second.end())
        {
            const auto threadId = std::this_thread::get_id();
            agent->second.emplace(changedTile, threadId);
            return threadId;
        }

        return tile->second;
    }

    void AsyncNavMeshUpdater::unlockTile(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile)
    {
        if (mSettings.get().mAsyncNavMeshUpdaterThreads <= 1)
            return;

        auto locked = mProcessingTiles.lock();

        auto agent = locked->find(agentHalfExtents);
        if (agent == locked->end())
            return;

        auto tile = agent->second.find(changedTile);
        if (tile == agent->second.end())
            return;

        agent->second.erase(tile);

        if (agent->second.empty())
            locked->erase(agent);

        if (locked->empty())
            mProcessed.notify_all();
    }

    std::size_t AsyncNavMeshUpdater::getTotalJobs() const
    {
        const std::scoped_lock lock(mMutex);
        return mJobs.size() + getTotalThreadJobsUnsafe();
    }

    std::size_t AsyncNavMeshUpdater::getTotalThreadJobsUnsafe() const
    {
        return std::accumulate(mThreadsQueues.begin(), mThreadsQueues.end(), std::size_t(0),
            [] (auto r, const auto& v) { return r + v.second.mJobs.size(); });
    }

    void AsyncNavMeshUpdater::cleanupLastUpdates()
    {
        const auto now = std::chrono::steady_clock::now();

        const std::lock_guard<std::mutex> lock(mMutex);

        for (auto agent = mLastUpdates.begin(); agent != mLastUpdates.end();)
        {
            for (auto tile = agent->second.begin(); tile != agent->second.end();)
            {
                if (now - tile->second > mSettings.get().mMinUpdateInterval)
                    tile = agent->second.erase(tile);
                else
                    ++tile;
            }

            if (agent->second.empty())
                agent = mLastUpdates.erase(agent);
            else
                ++agent;
        }
    }
}
