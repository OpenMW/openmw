#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H

#include "recastmesh.hpp"

#include <osg/Vec3f>

#include <boost/optional.hpp>

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

class dtNavMesh;

namespace DetourNavigator
{
    using NavMeshConstPtr = std::shared_ptr<const dtNavMesh>;

    struct NavMeshCacheItem
    {
        NavMeshConstPtr mValue = nullptr;
        std::size_t mRevision;

        NavMeshCacheItem(std::size_t mRevision)
            : mRevision(mRevision)
        {}
    };

    class AsyncNavMeshUpdater
    {
    public:
        AsyncNavMeshUpdater(const Settings& settings);
        ~AsyncNavMeshUpdater();

        void post(const osg::Vec3f& agentHalfExtents, const std::shared_ptr<RecastMesh>& recastMesh,
                  const std::shared_ptr<NavMeshCacheItem>& navMeshCacheItem);

        void wait();

    private:
        struct Job
        {
            osg::Vec3f mAgentHalfExtents;
            std::shared_ptr<RecastMesh> mRecastMesh;
            std::shared_ptr<NavMeshCacheItem> mNavMeshCacheItem;
        };

        using Jobs = std::map<osg::Vec3f, Job>;

        std::reference_wrapper<const Settings> mSettings;
        std::atomic_size_t mMaxRevision;
        std::atomic_bool mShouldStop;
        std::mutex mMutex;
        std::condition_variable mHasJob;
        std::condition_variable mDone;
        Jobs mJobs;
        std::thread mThread;

        void process() throw();

        void processJob(const Job& job);

        boost::optional<Job> getNextJob();

        void notifyHasJob();

        void writeDebugFiles(const Job& job) const;
    };
}

#endif
