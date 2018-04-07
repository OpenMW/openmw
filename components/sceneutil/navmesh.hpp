#ifndef OPENMW_COMPONENTS_SCENEUTIL_NAVMESH_H
#define OPENMW_COMPONENTS_SCENEUTIL_NAVMESH_H

#include <osg/ref_ptr>

class dtNavMesh;

namespace osg
{
    class Group;
}

namespace DetourNavigator
{
    struct Settings;
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createNavMeshGroup(const dtNavMesh& navMesh, const DetourNavigator::Settings& settings);
}

#endif
