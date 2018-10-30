#include "asyncnavmeshupdater.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "settings.hpp"

#include <components/debug/debuglog.hpp>

#include <iostream>

namespace
{
    using DetourNavigator::ChangeType;
    using DetourNavigator::TilePosition;

    int getManhattanDistance(const TilePosition& lhs, const TilePosition& rhs)
    {
        return std::abs(lhs.x() - rhs.x()) + std::abs(lhs.y() - rhs.y());
    }

    std::tuple<ChangeType, int, int> makePriority(const TilePosition& position, const ChangeType changeType,
                                                    const TilePosition& playerTile)
    {
        return std::make_tuple(
            changeType,
            getManhattanDistance(position, playerTile),
            getManhattanDistance(position, TilePosition {0, 0})
        );
    }
}

namespace DetourNavigator
{
    static std::ostream& operator <<(std::ostream& stream, UpdateNavMeshStatus value)
    {
        switch (value)
        {
            case UpdateNavMeshStatus::ignore:
                return stream << "ignore";
            case UpdateNavMeshStatus::removed:
                return stream << "removed";
            case UpdateNavMeshStatus::add:
                return stream << "add";
            case UpdateNavMeshStatus::replaced:
                return stream << "replaced";
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
                mJobs.push(Job {agentHalfExtents, navMeshCacheItem, changedTile.first,
                                makePriority(changedTile.first, changedTile.second, playerTile)});
        }

        log("posted ", mJobs.size(), " jobs");

        mHasJob.notify_all();
    }

    void AsyncNavMeshUpdater::wait()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mDone.wait(lock, [&] { return mJobs.empty(); });
    }

    void AsyncNavMeshUpdater::process() throw()
    {
        log("start process jobs");
        while (!mShouldStop)
        {
            try
            {
                if (const auto job = getNextJob())
                    processJob(*job);
            }
            catch (const std::exception& e)
            {
                DetourNavigator::log("AsyncNavMeshUpdater::process exception: ", e.what());
            }
        }
        log("stop process jobs");
    }

    void AsyncNavMeshUpdater::processJob(const Job& job)
    {
        log("process job for agent=", job.mAgentHalfExtents);

        const auto start = std::chrono::steady_clock::now();

        const auto firstStart = setFirstStart(start);

        const auto recastMesh = mRecastMeshManager.get().getMesh(job.mChangedTile);
        const auto playerTile = *mPlayerTile.lockConst();
        const auto offMeshConnections = mOffMeshConnectionsManager.get().get(job.mChangedTile);

        const auto status = updateNavMesh(job.mAgentHalfExtents, recastMesh.get(), job.mChangedTile, playerTile,
            offMeshConnections, mSettings, job.mNavMeshCacheItem, mNavMeshTilesCache);

        const auto finish = std::chrono::steady_clock::now();

        writeDebugFiles(job, recastMesh.get());

        using FloatMs = std::chrono::duration<float, std::milli>;

        const auto locked = job.mNavMeshCacheItem.lockConst();
        log("cache updated for agent=", job.mAgentHalfExtents, " status=", status,
            " generation=", locked->getGeneration(),
            " revision=", locked->getNavMeshRevision(),
            " time=", std::chrono::duration_cast<FloatMs>(finish - start).count(), "ms",
            " total_time=", std::chrono::duration_cast<FloatMs>(finish - firstStart).count(), "ms");
    }

    boost::optional<AsyncNavMeshUpdater::Job> AsyncNavMeshUpdater::getNextJob()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mJobs.empty())
            mHasJob.wait_for(lock, std::chrono::milliseconds(10));
        if (mJobs.empty())
        {
            mFirstStart.lock()->reset();
            mDone.notify_all();
            return boost::none;
        }
        log("got ", mJobs.size(), " jobs");
        const auto job = mJobs.top();
        mJobs.pop();
        const auto pushed = mPushed.find(job.mAgentHalfExtents);
        pushed->second.erase(job.mChangedTile);
        if (pushed->second.empty())
            mPushed.erase(pushed);
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
            writeToFile(job.mNavMeshCacheItem.lockConst()->getValue(), mSettings.get().mNavMeshPathPrefix, navMeshRevision);
    }

    std::chrono::steady_clock::time_point AsyncNavMeshUpdater::setFirstStart(const std::chrono::steady_clock::time_point& value)
    {
        const auto locked = mFirstStart.lock();
        if (!*locked)
            *locked = value;
        return *locked.get();
    }
}
