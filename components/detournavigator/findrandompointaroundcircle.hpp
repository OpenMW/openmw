#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDRANDOMPOINTAROUNDCIRCLE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDRANDOMPOINTAROUNDCIRCLE_H

#include "flags.hpp"

#include <optional>
#include <osg/Vec3f>

class dtNavMeshQuery;

namespace DetourNavigator
{
    struct DetourSettings;

    std::optional<osg::Vec3f> findRandomPointAroundCircle(const dtNavMeshQuery& navMeshQuery,
        const osg::Vec3f& halfExtents, const osg::Vec3f& start, const float maxRadius, const Flags includeFlags,
        float (*prng)());
}

#endif
