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

    void generateAllNavMeshTiles(const osg::Vec3f& agentHalfExtents, const DetourNavigator::Settings& settings,
        std::size_t threadsNumber, bool removeUnusedTiles, WorldspaceData& cellsData,
        DetourNavigator::NavMeshDb&& db);
}

#endif
