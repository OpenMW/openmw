#ifndef OPENMW_COMPONENTS_MISC_CONVERT_H
#define OPENMW_COMPONENTS_MISC_CONVERT_H

#include <components/esm/defs.hpp>
#include <components/esm/loadpgrd.hpp>

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

    inline osg::Vec3f makeOsgVec3f(const ESM::Pathgrid::Point& value)
    {
        return osg::Vec3f(value.mX, value.mY, value.mZ);
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

    inline osg::Quat makeOsgQuat(const float (&rotation)[3])
    {
        return osg::Quat(rotation[2], osg::Vec3f(0, 0, -1))
            * osg::Quat(rotation[1], osg::Vec3f(0, -1, 0))
            * osg::Quat(rotation[0], osg::Vec3f(-1, 0, 0));
    }

    inline osg::Quat makeOsgQuat(const ESM::Position& position)
    {
        return makeOsgQuat(position.rot);
    }

    inline btQuaternion makeBulletQuaternion(const float (&rotation)[3])
    {
        return btQuaternion(btVector3(0, 0, -1), rotation[2])
            * btQuaternion(btVector3(0, -1, 0), rotation[1])
            * btQuaternion(btVector3(-1, 0, 0), rotation[0]);
    }

    inline btQuaternion makeBulletQuaternion(const ESM::Position& position)
    {
        return makeBulletQuaternion(position.rot);
    }
}
}

#endif