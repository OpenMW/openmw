#include "asyncnavmeshupdater.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "settings.hpp"

#include <components/debug/debuglog.hpp>

#include <osg/Stats>

#include <numeric>

namespace
{
    using DetourNavigator::ChangeType;
    using DetourNavigator::TilePosition;

    int getManhattanDistance(const TilePosition& lhs, const TilePosition& rhs)
    {
        return std::abs(lhs.x() - rhs.x()) + std::abs(lhs.y() - rhs.y());
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
        }
        return stream << "unknown";
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
        *mPlayerTile.lock() = playerTile;

        if (changedTiles.empty())
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

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

                mJobs.push(std::move(job));
            }
        }

        Log(Debug::Debug) << "Posted " << mJobs.size() << " navigator jobs";

        if (!mJobs.empty())
            mHasJob.notify_all();
    }

    void AsyncNavMeshUpdater::wait()
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mDone.wait(lock, [&] { return mJobs.empty() && getTotalThreadJobsUnsafe() == 0; });
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

    void AsyncNavMeshUpdater::process() throw()
    {
        Log(Debug::Debug) << "Start process navigator jobs by thread=" << std::this_thread::get_id();
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

        const auto finish = std::chrono::steady_clock::now();

        writeDebugFiles(job, recastMesh.get());

        using FloatMs = std::chrono::duration<float, std::milli>;

        const auto locked = navMeshCacheItem->lockConst();
        Log(Debug::Debug) << std::fixed << std::setprecision(2) <<
            "Cache updated for agent=(" << job.mAgentHalfExtents << ")" <<
            " status=" << status <<
            " generation=" << locked->getGeneration() <<
            " revision=" << locked->getNavMeshRevision() <<
            " time=" << std::chrono::duration_cast<FloatMs>(finish - start).count() << "ms" <<
            " total_time=" << std::chrono::duration_cast<FloatMs>(finish - firstStart).count() << "ms"
            " thread=" << std::this_thread::get_id();

        return isSuccess(status);
    }

    boost::optional<AsyncNavMeshUpdater::Job> AsyncNavMeshUpdater::getNextJob()
    {
        std::unique_lock<std::mutex> lock(mMutex);

        const auto threadId = std::this_thread::get_id();
        auto& threadQueue = mThreadsQueues[threadId];

        while (true)
        {
            const auto hasJob = [&] { return !mJobs.empty() || !threadQueue.mJobs.empty(); };

            if (!mHasJob.wait_for(lock, std::chrono::milliseconds(10), hasJob))
            {
                mFirstStart.lock()->reset();
                if (getTotalThreadJobsUnsafe() == 0)
                    mDone.notify_all();
                return boost::none;
            }

            Log(Debug::Debug) << "Got " << mJobs.size() << " navigator jobs and "
                << threadQueue.mJobs.size() << " thread jobs by thread=" << std::this_thread::get_id();

            auto job = threadQueue.mJobs.empty()
                ? getJob(mJobs, mPushed)
                : getJob(threadQueue.mJobs, threadQueue.mPushed);

            const auto owner = lockTile(job.mAgentHalfExtents, job.mChangedTile);

            if (owner == threadId)
                return job;

            postThreadJob(std::move(job), mThreadsQueues[owner]);
        }
    }

    AsyncNavMeshUpdater::Job AsyncNavMeshUpdater::getJob(Jobs& jobs, Pushed& pushed)
    {
        auto job = jobs.top();
        jobs.pop();

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
            mJobs.push(std::move(job));
            mHasJob.notify_all();
        }
    }

    void AsyncNavMeshUpdater::postThreadJob(Job&& job, Queue& queue)
    {
        if (queue.mPushed[job.mAgentHalfExtents].insert(job.mChangedTile).second)
        {
            queue.mJobs.push(std::move(job));
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

    std::size_t AsyncNavMeshUpdater::getTotalThreadJobsUnsafe() const
    {
        return std::accumulate(mThreadsQueues.begin(), mThreadsQueues.end(), std::size_t(0),
            [] (auto r, const auto& v) { return r + v.second.mJobs.size(); });
    }
}
