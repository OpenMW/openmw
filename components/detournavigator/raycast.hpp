#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RAYCAST_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RAYCAST_H

#include "flags.hpp"

#include <optional>
#include <osg/Vec3f>

class dtNavMesh;

namespace DetourNavigator
{
    struct Settings;

    std::optional<osg::Vec3f> raycast(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents,
        const osg::Vec3f& start, const osg::Vec3f& end, const Flags includeFlags, const Settings& settings);
}

#endif
