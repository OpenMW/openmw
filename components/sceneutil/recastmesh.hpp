#ifndef OPENMW_COMPONENTS_SCENEUTIL_RECASTMESH_H
#define OPENMW_COMPONENTS_SCENEUTIL_RECASTMESH_H

#include <osg/ref_ptr>

namespace osg
{
    class Group;
    class StateSet;
}

namespace DetourNavigator
{
    class RecastMesh;
    struct RecastSettings;
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createRecastMeshGroup(const DetourNavigator::RecastMesh& recastMesh,
        const DetourNavigator::RecastSettings& settings, const osg::ref_ptr<osg::StateSet>& debugDrawStateSet);
}

#endif
