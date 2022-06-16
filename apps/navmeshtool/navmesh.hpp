#ifndef OPENMW_NAVMESHTOOL_NAVMESH_H
#define OPENMW_NAVMESHTOOL_NAVMESH_H

#include <osg/Vec3f>

#include <cstddef>

namespace DetourNavigator
{
    class NavMeshDb;
    struct Settings;
    struct AgentBounds;
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

    Status generateAllNavMeshTiles(const DetourNavigator::AgentBounds& agentBounds, const DetourNavigator::Settings& settings,
        std::size_t threadsNumber, bool removeUnusedTiles, bool writeBinaryLog, WorldspaceData& cellsData,
        DetourNavigator::NavMeshDb&& db);
}

#endif
