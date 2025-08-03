#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RAYCAST_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RAYCAST_H

#include "flags.hpp"

#include <osg/Vec3f>

#include <optional>

class dtNavMeshQuery;

namespace DetourNavigator
{
    struct DetourSettings;

    std::optional<osg::Vec3f> raycast(const dtNavMeshQuery& navMeshQuery, const osg::Vec3f& halfExtents,
        const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags);
}

#endif
