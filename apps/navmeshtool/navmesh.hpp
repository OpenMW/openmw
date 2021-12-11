#ifndef OPENMW_NAVMESHTOOL_NAVMESH_H
#define OPENMW_NAVMESHTOOL_NAVMESH_H

#include <osg/Vec3f>

#include <cstddef>
#include <string_view>

namespace DetourNavigator
{
    class NavMeshDb;
    struct Settings;
}

namespace NavMeshTool
{
    struct WorldspaceData;

    void generateAllNavMeshTiles(const osg::Vec3f& agentHalfExtents, const DetourNavigator::Settings& settings,
        const std::size_t threadsNumber, WorldspaceData& cellsData, DetourNavigator::NavMeshDb&& db);
}

#endif
