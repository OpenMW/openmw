#ifndef OPENMW_COMPONENTS_BULLETHELPERS_TRANSFORMBOUNDINGBOX_H
#define OPENMW_COMPONENTS_BULLETHELPERS_TRANSFORMBOUNDINGBOX_H

#include <LinearMath/btVector3.h>
#include <LinearMath/btTransform.h>

#include <algorithm>

namespace BulletHelpers
{
    inline btVector3 min(const btVector3& a, const btVector3& b)
    {
        return btVector3(std::min(a.x(), b.x()), std::min(a.y(), b.y()), std::min(a.z(), b.z()));
    }

    inline btVector3 max(const btVector3& a, const btVector3& b)
    {
        return btVector3(std::max(a.x(), b.x()), std::max(a.y(), b.y()), std::max(a.z(), b.z()));
    }

    // http://dev.theomader.com/transform-bounding-boxes/
    inline void transformBoundingBox(const btTransform& transform, btVector3& aabbMin, btVector3& aabbMax)
    {
        const btVector3 xa(transform.getBasis().getColumn(0) * aabbMin.x());
        const btVector3 xb(transform.getBasis().getColumn(0) * aabbMax.x());

        const btVector3 ya(transform.getBasis().getColumn(1) * aabbMin.y());
        const btVector3 yb(transform.getBasis().getColumn(1) * aabbMax.y());

        const btVector3 za(transform.getBasis().getColumn(2) * aabbMin.z());
        const btVector3 zb(transform.getBasis().getColumn(2) * aabbMax.z());

        aabbMin = min(xa, xb) + min(ya, yb) + min(za, zb) + transform.getOrigin();
        aabbMax = max(xa, xb) + max(ya, yb) + max(za, zb) + transform.getOrigin();
    }
}

#endif
