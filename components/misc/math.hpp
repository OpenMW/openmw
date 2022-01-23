#ifndef OPENMW_COMPONENTS_MISC_MATH_H
#define OPENMW_COMPONENTS_MISC_MATH_H

#include <osg/Vec3f>

namespace Misc
{
    inline osg::Vec3f getVectorToLine(const osg::Vec3f& position, const osg::Vec3f& a, const osg::Vec3f& b)
    {
        osg::Vec3f direction = b - a;
        direction.normalize();
        return (position - a) ^ direction;
    }
}

#endif
