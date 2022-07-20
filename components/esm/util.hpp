#ifndef OPENMW_ESM_UTIL_H
#define OPENMW_ESM_UTIL_H

#include <osg/Vec3f>
#include <osg/Quat>

namespace ESM
{

// format 0, savegames only

struct Quaternion
{
    float mValues[4];

    Quaternion() = default;

    Quaternion(const osg::Quat& q)
    {
        mValues[0] = q.w();
        mValues[1] = q.x();
        mValues[2] = q.y();
        mValues[3] = q.z();
    }

    operator osg::Quat () const
    {
        return osg::Quat(mValues[1], mValues[2], mValues[3], mValues[0]);
    }
};

struct Vector3
{
    float mValues[3];

    Vector3() = default;

    Vector3(const osg::Vec3f& v)
    {
        mValues[0] = v.x();
        mValues[1] = v.y();
        mValues[2] = v.z();
    }

    operator osg::Vec3f () const
    {
        return osg::Vec3f(mValues[0], mValues[1], mValues[2]);
    }
};

}

#endif
