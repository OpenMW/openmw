#ifndef OPENMW_COMPONENTS_MISC_CONVERT_H
#define OPENMW_COMPONENTS_MISC_CONVERT_H

#include <LinearMath/btTransform.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>
#include <osg/Vec3f>
#include <osg/Quat>

namespace Misc
{
namespace Convert
{
    inline osg::Vec3f makeOsgVec3f(const float* values)
    {
        return osg::Vec3f(values[0], values[1], values[2]);
    }

    inline osg::Vec3f makeOsgVec3f(const btVector3& value)
    {
        return osg::Vec3f(value.x(), value.y(), value.z());
    }

    inline btVector3 toBullet(const osg::Vec3f& vec)
    {
        return btVector3(vec.x(), vec.y(), vec.z());
    }

    inline btQuaternion toBullet(const osg::Quat& quat)
    {
        return btQuaternion(quat.x(), quat.y(), quat.z(), quat.w());
    }

    inline osg::Vec3f toOsg(const btVector3& vec)
    {
        return osg::Vec3f(vec.x(), vec.y(), vec.z());
    }

    inline osg::Quat toOsg(const btQuaternion& quat)
    {
        return osg::Quat(quat.x(), quat.y(), quat.z(), quat.w());
    }
}
}

#endif