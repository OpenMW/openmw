#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDRANDOMPOINTAROUNDCIRCLE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FINDRANDOMPOINTAROUNDCIRCLE_H

#include "flags.hpp"

#include <boost/optional.hpp>

#include <osg/Vec3f>

class dtNavMesh;

namespace DetourNavigator
{
    struct Settings;

    boost::optional<osg::Vec3f> findRandomPointAroundCircle(const dtNavMesh& navMesh, const osg::Vec3f& halfExtents,
        const osg::Vec3f& start, const float maxRadius, const Flags includeFlags, const Settings& settings);
}

#endif
