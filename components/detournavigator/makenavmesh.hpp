#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_MAKENAVMESH_H

#include <osg/Vec3f>

#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    class RecastMesh;
    struct Settings;

    using NavMeshPtr = std::shared_ptr<dtNavMesh>;

    NavMeshPtr makeNavMesh(const osg::Vec3f& agentHalfExtents, const RecastMesh& recastMesh, const Settings& settings);
}

#endif
