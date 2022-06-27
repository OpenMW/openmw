#ifndef OPENMW_COMPONENTS_BULLETHELPERS_OPERATORS_H
#define OPENMW_COMPONENTS_BULLETHELPERS_OPERATORS_H

#include <tuple>

#include <LinearMath/btVector3.h>
#include <LinearMath/btTransform.h>

inline bool operator <(const btVector3& lhs, const btVector3& rhs)
{
    return std::tie(lhs.x(), lhs.y(), lhs.z()) < std::tie(rhs.x(), rhs.y(), rhs.z());
}

inline bool operator <(const btMatrix3x3& lhs, const btMatrix3x3& rhs)
{
    return std::tie(lhs[0], lhs[1], lhs[2]) < std::tie(rhs[0], rhs[1], rhs[2]);
}

inline bool operator <(const btTransform& lhs, const btTransform& rhs)
{
    return std::tie(lhs.getBasis(), lhs.getOrigin()) < std::tie(rhs.getBasis(), rhs.getOrigin());
}

#endif
