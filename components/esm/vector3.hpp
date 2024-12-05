#ifndef OPENMW_COMPONENTS_ESM_VECTOR3_H
#define OPENMW_COMPONENTS_ESM_VECTOR3_H

#include <osg/Vec3f>

namespace ESM
{
    // format 0, savegames only
    struct Vector3
    {
        float mValues[3];

        Vector3() = default;

        Vector3(const osg::Vec3f& v)
            : mValues{ v.x(), v.y(), v.z() }
        {
        }

        operator osg::Vec3f() const { return osg::Vec3f(mValues[0], mValues[1], mValues[2]); }
    };
}

#endif
