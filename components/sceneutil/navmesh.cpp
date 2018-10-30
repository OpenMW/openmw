#include "navmesh.hpp"
#include "detourdebugdraw.hpp"

#include <components/detournavigator/settings.hpp>

#include <DetourDebugDraw.h>

#include <osg/Group>

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createNavMeshGroup(const dtNavMesh& navMesh, const DetourNavigator::Settings& settings)
    {
        const osg::ref_ptr<osg::Group> group(new osg::Group);
        DebugDraw debugDraw(*group, osg::Vec3f(0, 0, 10), 1.0f / settings.mRecastScaleFactor);
        dtNavMeshQuery navMeshQuery;
        navMeshQuery.init(&navMesh, settings.mMaxNavMeshQueryNodes);
        duDebugDrawNavMeshWithClosedList(&debugDraw, navMesh, navMeshQuery,
                                         DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST);
        return group;
    }
}
