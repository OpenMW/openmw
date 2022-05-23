#ifndef OPENMW_NAVMESHTOOL_NAVMESH_H
#define OPENMW_NAVMESHTOOL_NAVMESH_H

#include <osg/Vec3f>

#include <cstddef>

namespace DetourNavigator
{
    class NavMeshDb;
    struct Settings;
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

    Status generateAllNavMeshTiles(const osg::Vec3f& agentHalfExtents, const DetourNavigator::Settings& settings,
        std::size_t threadsNumber, bool removeUnusedTiles, bool writeBinaryLog, WorldspaceData& cellsData,
        DetourNavigator::NavMeshDb&& db);
}

#endif
