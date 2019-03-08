#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H

#include "navmeshcacheitem.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "tilecachedrecastmeshmanager.hpp"
#include "tileposition.hpp"
#include "navmeshtilescache.hpp"

#include <osg/Vec3f>

#include <boost/optional.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>

class dtNavMesh;

namespace DetourNavigator
{
    enum class ChangeType
    {
        remove = 0,
        mixed = 1,
        add = 2,
        update = 3,
    };

    class AsyncNavMeshUpdater
    {
    public:
        AsyncNavMeshUpdater(const Settings& settings, TileCachedRecastMeshManager& recastMeshManager,
            OffMeshConnectionsManager& offMeshConnectionsManager);
        ~AsyncNavMeshUpdater();

        void post(const osg::Vec3f& agentHalfExtents, const SharedNavMeshCacheItem& mNavMeshCacheItem,
            const TilePosition& playerTile, const std::map<TilePosition, ChangeType>& changedTiles);

        void wait();

    private:
        struct Job
        {
            osg::Vec3f mAgentHalfExtents;
            SharedNavMeshCacheItem mNavMeshCacheItem;
            TilePosition mChangedTile;
            unsigned mTryNumber;
            ChangeType mChangeType;
            int mDistanceToPlayer;
            int mDistanceToOrigin;

            std::tuple<unsigned, ChangeType, int, int> getPriority() const
            {
                return std::make_tuple(mTryNumber, mChangeType, mDistanceToPlayer, mDistanceToOrigin);
            }

            friend inline bool operator <(const Job& lhs, const Job& rhs)
            {
                return lhs.getPriority() > rhs.getPriority();
            }
        };

        using Jobs = std::priority_queue<Job, std::deque<Job>>;

        std::reference_wrapper<const Settings> mSettings;
        std::reference_wrapper<TileCachedRecastMeshManager> mRecastMeshManager;
        std::reference_wrapper<OffMeshConnectionsManager> mOffMeshConnectionsManager;
        std::atomic_bool mShouldStop;
        std::mutex mMutex;
        std::condition_variable mHasJob;
        std::condition_variable mDone;
        Jobs mJobs;
        std::map<osg::Vec3f, std::set<TilePosition>> mPushed;
        Misc::ScopeGuarded<TilePosition> mPlayerTile;
        Misc::ScopeGuarded<boost::optional<std::chrono::steady_clock::time_point>> mFirstStart;
        NavMeshTilesCache mNavMeshTilesCache;
        std::vector<std::thread> mThreads;

        void process() throw();

        bool processJob(const Job& job);

        boost::optional<Job> getNextJob();

        void writeDebugFiles(const Job& job, const RecastMesh* recastMesh) const;

        std::chrono::steady_clock::time_point setFirstStart(const std::chrono::steady_clock::time_point& value);

        void repost(Job&& job);
    };
}

#endif
