#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTPARAMS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTPARAMS_H

#include "agentbounds.hpp"

#include <osg/Vec3f>

#include <cassert>
#include <cmath>
#include <algorithm>

namespace DetourNavigator
{
    inline float getAgentHeight(const AgentBounds& agentBounds)
    {
        return 2.0f * agentBounds.mHalfExtents.z();
    }

    inline float getAgentRadius(const AgentBounds& agentBounds)
    {
        switch (agentBounds.mShapeType)
        {
            case CollisionShapeType::Aabb:
                return std::max(agentBounds.mHalfExtents.x(), agentBounds.mHalfExtents.y()) * std::sqrt(2);
            case CollisionShapeType::RotatingBox:
                return agentBounds.mHalfExtents.x();
        }
        assert(false && "Unsupported agent shape type");
        return 0;
    }
}

#endif
