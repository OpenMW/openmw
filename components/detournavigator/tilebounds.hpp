#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILEBOUNDS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILEBOUNDS_H

#include <osg/Vec2f>

namespace DetourNavigator
{
    struct TileBounds
    {
        osg::Vec2f mMin;
        osg::Vec2f mMax;
    };
}

#endif
