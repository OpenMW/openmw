#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTION_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTION_H

#include "areatype.hpp"

#include <osg/Vec3f>

#include <tuple>

namespace DetourNavigator
{
    struct OffMeshConnection
    {
        osg::Vec3f mStart;
        osg::Vec3f mEnd;
        AreaType mAreaType;
    };

    inline bool operator<(const OffMeshConnection& lhs, const OffMeshConnection& rhs)
    {
        return std::tie(lhs.mStart, lhs.mEnd, lhs.mAreaType) < std::tie(rhs.mStart, rhs.mEnd, rhs.mAreaType);
    }
}

#endif
