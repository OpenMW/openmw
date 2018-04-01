#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H

#include "recastmesh.hpp"
#include "sharednavmesh.hpp"
#include "tileposition.hpp"

#include <osg/Vec3f>

#include <boost/optional.hpp>

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>

class dtNavMesh;

namespace DetourNavigator
{
    struct NavMeshCacheItem
    {
        SharedNavMesh mValue;
        std::size_t mRevision;
    };

    class AsyncNavMeshUpdater
    {
    public:
        AsyncNavMeshUpdater(const Settings& settings);
        ~AsyncNavMeshUpdater();

        void post(const osg::Vec3f& agentHalfExtents, const std::shared_ptr<RecastMesh>& recastMesh,
            const std::shared_ptr<NavMeshCacheItem>& navMeshCacheItem, const TilePosition& playerTile,
            const std::set<TilePosition>& changedTiles);

        void wait();

    private:
        struct Job
        {
            osg::Vec3f mAgentHalfExtents;
            std::shared_ptr<RecastMesh> mRecastMesh;
            std::shared_ptr<NavMeshCacheItem> mNavMeshCacheItem;
            TilePosition mChangedTile;
            std::pair<int, int> mPriority;

            friend inline bool operator <(const Job& lhs, const Job& rhs)
            {
                return lhs.mPriority > rhs.mPriority;
            }
        };

        using Jobs = std::priority_queue<Job, std::deque<Job>>;

        std::reference_wrapper<const Settings> mSettings;
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
