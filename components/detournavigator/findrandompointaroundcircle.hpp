#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDRANDOMPOINTAROUNDCIRCLE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDRANDOMPOINTAROUNDCIRCLE_H

#include "flags.hpp"

#include <osg/Vec3f>

#include <optional>

class dtNavMeshQuery;

namespace DetourNavigator
{
    struct DetourSettings;

    std::optional<osg::Vec3f> findRandomPointAroundCircle(const dtNavMeshQuery& navMeshQuery,
        const osg::Vec3f& halfExtents, const osg::Vec3f& start, const float maxRadius, const Flags includeFlags,
        float (*prng)());
}

#endif
