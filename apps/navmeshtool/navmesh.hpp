#ifndef OPENMW_NAVMESHTOOL_NAVMESH_H
#define OPENMW_NAVMESHTOOL_NAVMESH_H

namespace DetourNavigator
{
    class NavMeshDb;
    struct Settings;
    struct AgentBounds;
}

namespace SceneUtil
{
    class WorkQueue;
}

namespace NavMeshTool
{
    struct WorldspaceData;

    enum class Status
    {
        Ok,
        Cancelled,
        NotEnoughSpace,
    };

    struct Result
    {
        Status mStatus;
        bool mNeedVacuum;
    };

    Result generateAllNavMeshTiles(const DetourNavigator::AgentBounds& agentBounds,
        const DetourNavigator::Settings& settings, bool removeUnusedTiles, bool writeBinaryLog,
        const WorldspaceData& data, DetourNavigator::NavMeshDb& db, SceneUtil::WorkQueue& workQueue);
}

#endif
