#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTION_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTION_H

#include <osg/Vec3f>

namespace DetourNavigator
{
    struct OffMeshConnection
    {
        osg::Vec3f mStart;
        osg::Vec3f mEnd;
    };
}

#endif
