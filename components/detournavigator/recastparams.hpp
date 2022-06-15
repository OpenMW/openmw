#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTPARAMS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTPARAMS_H

#include <osg/Vec3f>

#include <cmath>

namespace DetourNavigator
{
    inline float getAgentHeight(const osg::Vec3f& agentHalfExtents)
    {
        return 2.0f * agentHalfExtents.z();
    }

    inline float getAgentRadius(const osg::Vec3f& agentHalfExtents)
    {
        return std::max(agentHalfExtents.x(), agentHalfExtents.y()) * std::sqrt(2);
    }
}

#endif
