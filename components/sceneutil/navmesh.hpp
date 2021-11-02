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
    osg::ref_ptr<osg::StateSet> makeNavMeshTileStateSet();

    osg::ref_ptr<osg::Group> createNavMeshTileGroup(const dtNavMesh& navMesh, const dtMeshTile& meshTile,
        const DetourNavigator::Settings& settings, const osg::ref_ptr<osg::StateSet>& groupStateSet,
        const osg::ref_ptr<osg::StateSet>& debugDrawStateSet);
}

#endif
