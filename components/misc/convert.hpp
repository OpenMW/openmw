#ifndef OPENMW_COMPONENTS_MISC_CONVERT_H
#define OPENMW_COMPONENTS_MISC_CONVERT_H

#include <components/esm/position.hpp>
#include <components/esm3/loadpgrd.hpp>

#include <components/misc/float16.hpp>

#include <LinearMath/btQuaternion.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btVector3.h>
#include <osg/Matrixf>
#include <osg/Quat>
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <type_traits>

#include <bit>
#include <cstdint>

namespace Misc::Convert
{
    inline osg::Vec3f makeOsgVec3f(const float* values)
    {
        return osg::Vec3f(values[0], values[1], values[2]);
    }

    inline osg::Vec3f makeOsgVec3f(const ESM::Pathgrid::Point& value)
    {
        return osg::Vec3f(static_cast<float>(value.mX), static_cast<float>(value.mY), static_cast<float>(value.mZ));
    }

    // Vertices and UV sets may be half-precision.
    // OSG doesn't have a way to pass half-precision data at the moment.
    inline float halfToFloat(const Misc::float16_t value)
    {
        uint32_t bits = static_cast<uint32_t>(value & 0x8000) << 16;

        const uint32_t exp16 = (value & 0x7c00) >> 10;
        uint32_t frac16 = value & 0x3ff;
        if (exp16)
            bits |= (exp16 + 0x70) << 23;
        else if (frac16)
        {
            uint8_t offset = 0;
            do
            {
                ++offset;
                frac16 <<= 1;
            } while ((frac16 & 0x400) != 0x400);
            frac16 &= 0x3ff;
            bits |= (0x71 - offset) << 23;
        }
        bits |= frac16 << 13;

        return std::bit_cast<float>(bits);
    }

    inline osg::Vec3f makeOsgVec3f(const Misc::float16_t* values)
    {
        return osg::Vec3f(halfToFloat(values[0]), halfToFloat(values[1]), halfToFloat(values[2]));
    }

    inline btVector3 toBullet(const osg::Vec3f& vec)
    {
        return btVector3(vec.x(), vec.y(), vec.z());
    }

    inline btQuaternion toBullet(const osg::Quat& quat)
    {
        return btQuaternion(quat.x(), quat.y(), quat.z(), quat.w());
    }

    inline osg::Vec3d toOsg(const btVector3& vec)
    {
        return osg::Vec3d(vec.x(), vec.y(), vec.z());
    }

    inline osg::Vec3f makeOsgVec3f(const btVector3& vec)
    {
        return toOsg(vec);
    }

    inline osg::Quat toOsg(const btQuaternion& quat)
    {
        return osg::Quat(quat.x(), quat.y(), quat.z(), quat.w());
    }

    inline osg::Quat makeOsgQuat(const float (&rotation)[3])
    {
        return osg::Quat(rotation[2], osg::Vec3f(0, 0, -1)) * osg::Quat(rotation[1], osg::Vec3f(0, -1, 0))
            * osg::Quat(rotation[0], osg::Vec3f(-1, 0, 0));
    }

    inline osg::Quat makeOsgQuat(const ESM::Position& position)
    {
        return makeOsgQuat(position.rot);
    }

    inline btQuaternion makeBulletQuaternion(const float (&rotation)[3])
    {
        return btQuaternion(btVector3(0, 0, -1), rotation[2]) * btQuaternion(btVector3(0, -1, 0), rotation[1])
            * btQuaternion(btVector3(-1, 0, 0), rotation[0]);
    }

    inline btQuaternion makeBulletQuaternion(const ESM::Position& position)
    {
        return makeBulletQuaternion(position.rot);
    }

    inline btTransform makeBulletTransform(const ESM::Position& position)
    {
        return btTransform(makeBulletQuaternion(position), toBullet(position.asVec3()));
    }

    inline osg::Vec2f toOsgXY(const btVector3& value)
    {
        return osg::Vec2f(static_cast<float>(value.x()), static_cast<float>(value.y()));
    }

    // OSG <-> glm converters.
    //
    // OSG stores osg::Matrix row-major with row-vector convention (v * M);
    // glm stores glm::mat4 column-major with column-vector convention (M * v).
    // Row-major-times-row-vector and column-major-times-column-vector cancel
    // out: the two representations share the same 16-float memory image and
    // encode the same transformation, so make_mat4 and Matrixf(ptr) round-trip
    // exactly WITHOUT a transpose, provided the caller also flips the
    // multiplication order to match the new convention. This is covered by
    // MiscConvertGlm.MatrixTranslationPreservesTransform and
    // MiscConvertGlm.MatrixRotationPreservesTransform.
    static_assert(sizeof(glm::mat4) == sizeof(osg::Matrixf));
    static_assert(std::is_same_v<glm::mat4::value_type, osg::Matrixf::value_type>);
    static_assert(GLM_CONFIG_CTOR_INIT != GLM_CTOR_INIT_DISABLE, "build with GLM_FORCE_CTOR_INIT, see CMakeLists.txt");

    inline glm::vec2 toGlm(const osg::Vec2f& v)
    {
        return glm::vec2(v.x(), v.y());
    }

    inline osg::Vec2f toOsg(const glm::vec2& v)
    {
        return osg::Vec2f(v.x, v.y);
    }

    inline glm::vec3 toGlm(const osg::Vec3f& v)
    {
        return glm::vec3(v.x(), v.y(), v.z());
    }

    inline osg::Vec3f toOsg(const glm::vec3& v)
    {
        return osg::Vec3f(v.x, v.y, v.z);
    }

    inline glm::vec4 toGlm(const osg::Vec4f& v)
    {
        return glm::vec4(v.x(), v.y(), v.z(), v.w());
    }

    inline osg::Vec4f toOsg(const glm::vec4& v)
    {
        return osg::Vec4f(v.x, v.y, v.z, v.w);
    }

    inline glm::mat4 toGlm(const osg::Matrixf& m)
    {
        return glm::make_mat4(m.ptr());
    }

    inline osg::Matrixf toOsg(const glm::mat4& m)
    {
        return osg::Matrixf(glm::value_ptr(m));
    }

    inline glm::quat toGlm(const osg::Quat& q)
    {
        return glm::quat(
            static_cast<float>(q.w()), static_cast<float>(q.x()), static_cast<float>(q.y()), static_cast<float>(q.z()));
    }

    inline osg::Quat toOsg(const glm::quat& q)
    {
        return osg::Quat(q.x, q.y, q.z, q.w);
    }

    // Composes the same rotation as makeOsgQuat but returns it as a glm::quat. Handy at
    // Lua boundaries where the mod-facing value type is a glm::quat.
    inline glm::quat makeGlmQuat(const float (&rotation)[3])
    {
        return toGlm(makeOsgQuat(rotation));
    }

    inline glm::quat makeGlmQuat(const ESM::Position& position)
    {
        return makeGlmQuat(position.rot);
    }
}

#endif
