#ifndef OPENMW_NAVMESHTOOL_NAVMESH_H
#define OPENMW_NAVMESHTOOL_NAVMESH_H

#include <cstddef>
#include <memory>

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
    class NavMeshTileConsumer;

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
        std::size_t mProvided;
        std::size_t mInserted;
        std::size_t mUpdated;
        std::size_t mDeleted;
        GenerateTilesStats mStats;
    };

    class NavMeshTilesGenerator
    {
    public:
        NavMeshTilesGenerator(const DetourNavigator::AgentBounds& agentBounds,
            const DetourNavigator::Settings& settings, const GenerateAllNavMeshTilesOptions& options,
            DetourNavigator::NavMeshDb& db, SceneUtil::WorkQueue& workQueue);

        Status addWorldspace(WorldspaceData&& data);

        GenerateTilesResult finish();

    private:
        const DetourNavigator::AgentBounds& mAgentBounds;
        const DetourNavigator::Settings& mSettings;
        const GenerateAllNavMeshTilesOptions mOptions;
        SceneUtil::WorkQueue& mWorkQueue;
        std::shared_ptr<NavMeshTileConsumer> mConsumer;
    };
}

#endif
