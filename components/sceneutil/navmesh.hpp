#ifndef OPENMW_COMPONENTS_SCENEUTIL_NAVMESH_H
#define OPENMW_COMPONENTS_SCENEUTIL_NAVMESH_H

#include <osg/ref_ptr>

class dtNavMesh;
struct dtMeshTile;

namespace osg
{
    class Group;
    class StateSet;
}

namespace DetourNavigator
{
    struct Settings;
}

namespace SceneUtil
{
    enum NavMeshTileDrawFlags : unsigned char
    {
        NavMeshTileDrawFlagsOffMeshConnections = 1,
        NavMeshTileDrawFlagsClosedList = 1 << 1,
        NavMeshTileDrawFlagsColorTiles = 1 << 2,
        NavMeshTileDrawFlagsHeat = 1 << 3,
    };

    osg::ref_ptr<osg::Group> createNavMeshTileGroup(const dtNavMesh& navMesh, const dtMeshTile& meshTile,
        const DetourNavigator::Settings& settings, const osg::ref_ptr<osg::StateSet>& debugDrawStateSet,
        unsigned char flags, unsigned minSalt, unsigned maxSalt);
}

#endif
