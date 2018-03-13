#include "asyncnavmeshupdater.hpp"
#include "debug.hpp"
#include "makenavmesh.hpp"
#include "settings.hpp"

#include <components/debug/debuglog.hpp>

#include <iostream>

namespace DetourNavigator
{
    AsyncNavMeshUpdater::AsyncNavMeshUpdater(const Settings& settings)
        : mSettings(settings)
        , mMaxRevision(0)
        , mShouldStop()
        , mThread([&] { process(); })
    {
    }

    AsyncNavMeshUpdater::~AsyncNavMeshUpdater()
    {
        mShouldStop = true;
        std::unique_lock<std::mutex> lock(mMutex);
        mJobs.clear();
        mHasJob.notify_all();
        lock.unlock();
        mThread.join();
    }

    void AsyncNavMeshUpdater::post(const osg::Vec3f& agentHalfExtents, const std::shared_ptr<RecastMesh>& recastMesh,
        const std::shared_ptr<NavMeshCacheItem>& navMeshCacheItem)
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        mJobs[agentHalfExtents] = Job {agentHalfExtents, recastMesh, navMeshCacheItem};
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
                ::Log(Debug::Error) << "Exception while process navmesh updated job: " << e.what();
            }
        }
        log("stop process jobs");
    }

    void AsyncNavMeshUpdater::processJob(const Job& job)
    {
        log("process job for agent=", job.mAgentHalfExtents,
            " revision=", job.mNavMeshCacheItem->mRevision,
            " max_revision=", mMaxRevision);

        if (job.mNavMeshCacheItem->mRevision < mMaxRevision)
            return;

        mMaxRevision = job.mNavMeshCacheItem->mRevision;

        const auto start = std::chrono::steady_clock::now();

        job.mNavMeshCacheItem->mValue = makeNavMesh(job.mAgentHalfExtents, *job.mRecastMesh, mSettings);

        const auto finish = std::chrono::steady_clock::now();

        writeDebugFiles(job);

        using FloatMs = std::chrono::duration<float, std::milli>;

        log("cache updated for agent=", job.mAgentHalfExtents,
            " time=", std::chrono::duration_cast<FloatMs>(finish - start).count(), "ms");
    }

    boost::optional<AsyncNavMeshUpdater::Job> AsyncNavMeshUpdater::getNextJob()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mJobs.empty())
            mHasJob.wait_for(lock, std::chrono::milliseconds(10));
        if (mJobs.empty())
        {
            mDone.notify_all();
            return boost::none;
        }
        log("got ", mJobs.size(), " jobs");
        const auto job = mJobs.begin()->second;
        mJobs.erase(mJobs.begin());
        return job;
    }

    void AsyncNavMeshUpdater::writeDebugFiles(const Job& job) const
    {
#ifdef OPENMW_WRITE_TO_FILE
        const auto revision = std::to_string((std::chrono::steady_clock::now()
            - std::chrono::steady_clock::time_point()).count());
        writeToFile(*job.mRecastMesh, revision);
        writeToFile(*job.mNavMeshCacheItem->mValue, revision);
#endif
    }
}
