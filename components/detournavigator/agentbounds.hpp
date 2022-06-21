#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_AGENTBOUNDS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_AGENTBOUNDS_H

#include "collisionshapetype.hpp"

#include <osg/Vec3f>

#include <tuple>

namespace DetourNavigator
{
    struct AgentBounds
    {
        CollisionShapeType mShapeType;
        osg::Vec3f mHalfExtents;
    };

    inline auto tie(const AgentBounds& value)
    {
        return std::tie(value.mShapeType, value.mHalfExtents);
    }

    inline bool operator==(const AgentBounds& lhs, const AgentBounds& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline bool operator<(const AgentBounds& lhs, const AgentBounds& rhs)
    {
        return tie(lhs) < tie(rhs);
    }
}

#endif
