#ifndef OPENMW_MWPHYSICS_CONVERT_H
#define OPENMW_MWPHYSICS_CONVERT_H

#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

#include <osg/Vec3f>
#include <osg/Quat>

namespace MWPhysics
{

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

#endif
