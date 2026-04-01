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

    struct GenerateAllNavMeshTilesOptions
    {
        bool mRemoveUnusedTiles;
        bool mWriteBinaryLog;
        bool mCollectStats;
    };

    enum class Status
    {
        Ok,
        Cancelled,
        NotEnoughSpace,
    };

    struct GenerateTilesStats
    {
        int mMaxPolyCountPerTile = 0;
    };

    struct GenerateTilesResult
    {
        Status mStatus;
        bool mNeedVacuum;
        GenerateTilesStats mStats;
    };

    GenerateTilesResult generateAllNavMeshTiles(const DetourNavigator::AgentBounds& agentBounds,
        const DetourNavigator::Settings& settings, const GenerateAllNavMeshTilesOptions& options,
        const WorldspaceData& data, DetourNavigator::NavMeshDb& db, SceneUtil::WorkQueue& workQueue);
}

#endif
