#ifndef MISC_MATHUTIL_H
#define MISC_MATHUTIL_H

#include <components/misc/convert.hpp>

#include <osg/Math>
#include <osg/Matrixf>
#include <osg/Quat>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cmath>
#include <type_traits>

namespace Misc
{
    /// Normalizes given angle to the range [-PI, PI]. E.g. PI*3/2 -> -PI/2.
    inline double normalizeAngle(double angle)
    {
        double fullTurns = angle / (2 * osg::PI) + 0.5;
        return (fullTurns - floor(fullTurns) - 0.5) * (2 * osg::PI);
    }

    /// Rotates given 2d vector counterclockwise. Angle is in radians.
    inline osg::Vec2f rotateVec2f(osg::Vec2f vec, float angle)
    {
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        return osg::Vec2f(c * vec.x() - s * vec.y(), s * vec.x() + c * vec.y());
    }

    inline osg::Vec3f toEulerAnglesXZ(osg::Vec3f forward)
    {
        float x = -std::asin(forward.z());
        float z = std::atan2(forward.x(), forward.y());
        return osg::Vec3f(x, 0, z);
    }

    inline osg::Vec3f toEulerAnglesXZ(const glm::quat& quat)
    {
        const glm::vec3 forward = glm::normalize(quat * glm::vec3(0, 1, 0));
        return toEulerAnglesXZ(Misc::Convert::toOsg(forward));
    }

    inline osg::Vec3f toEulerAnglesXZ(const osg::Quat& quat)
    {
        return toEulerAnglesXZ(Misc::Convert::toGlm(quat));
    }

    inline osg::Vec3f toEulerAnglesXZ(const glm::mat4& m)
    {
        // OSG stores row-major with row-vector semantics: row 1 is the image of the
        // local +Y axis under M. In glm::make_mat4(osg.ptr()) column-major storage,
        // that same row is column 1.
        const glm::vec3 forward = glm::normalize(glm::vec3(m[1]));
        return toEulerAnglesXZ(Misc::Convert::toOsg(forward));
    }

    inline osg::Vec3f toEulerAnglesXZ(const osg::Matrixf& m)
    {
        return toEulerAnglesXZ(Misc::Convert::toGlm(m));
    }

    inline osg::Vec3f toEulerAnglesZYX(osg::Vec3f forward, osg::Vec3f up)
    {
        const float y = -std::asin(up.x());
        const float x = std::atan2(up.y(), up.z());
        // OSG's Quat::operator*(Quat) is Hamilton-reversed w.r.t. glm's:
        // OSG's (q1*q2)*v applies q1 first, then q2; glm's applies q2 first.
        // Preserve the original composition order (X then Y) by swapping the
        // operands in glm.
        const glm::quat q = glm::angleAxis(y, glm::vec3(0, 1, 0)) * glm::angleAxis(x, glm::vec3(1, 0, 0));
        const glm::vec3 forwardZ = q * Misc::Convert::toGlm(forward);
        const float z = std::atan2(forwardZ.x, forwardZ.y);
        return osg::Vec3f(x, y, z);
    }

    inline osg::Vec3f toEulerAnglesZYX(const glm::quat& quat)
    {
        const glm::vec3 forward = glm::normalize(quat * glm::vec3(0, 1, 0));
        const glm::vec3 up = glm::normalize(quat * glm::vec3(0, 0, 1));
        return toEulerAnglesZYX(Misc::Convert::toOsg(forward), Misc::Convert::toOsg(up));
    }

    inline osg::Vec3f toEulerAnglesZYX(const osg::Quat& quat)
    {
        return toEulerAnglesZYX(Misc::Convert::toGlm(quat));
    }

    inline osg::Vec3f toEulerAnglesZYX(const glm::mat4& m)
    {
        const glm::vec3 forward = glm::normalize(glm::vec3(m[1]));
        const glm::vec3 up = glm::normalize(glm::vec3(m[2]));
        return toEulerAnglesZYX(Misc::Convert::toOsg(forward), Misc::Convert::toOsg(up));
    }

    inline osg::Vec3f toEulerAnglesZYX(const osg::Matrixf& m)
    {
        return toEulerAnglesZYX(Misc::Convert::toGlm(m));
    }

    template <class T>
    bool isPowerOfTwo(T x)
    {
        static_assert(std::is_integral_v<T>);
        return ((x > 0) && ((x & (x - 1)) == 0));
    }

    inline int nextPowerOfTwo(int v)
    {
        if (isPowerOfTwo(v))
            return v;
        int depth = 0;
        while (v)
        {
            v >>= 1;
            depth++;
        }
        return 1 << depth;
    }
}

#endif
