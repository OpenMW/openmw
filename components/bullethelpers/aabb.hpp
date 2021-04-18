#ifndef OPENMW_COMPONENTS_BULLETHELPERS_AABB_H
#define OPENMW_COMPONENTS_BULLETHELPERS_AABB_H

#include <LinearMath/btVector3.h>
#include <LinearMath/btTransform.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletCollision/Gimpact/btBoxCollision.h>

inline bool operator==(const btAABB& lhs, const btAABB& rhs)
{
    return lhs.m_min == rhs.m_min && lhs.m_max == rhs.m_max;
}

inline bool operator!=(const btAABB& lhs, const btAABB& rhs)
{
    return !(lhs == rhs);
}

namespace BulletHelpers
{
    inline btAABB getAabb(const btCollisionShape& shape, const btTransform& transform)
    {
        btAABB result;
        shape.getAabb(transform, result.m_min, result.m_max);
        return result;
    }
}

#endif
