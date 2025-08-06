#ifndef COMPONENTS_LUA_UTILPACKAGE_H
#define COMPONENTS_LUA_UTILPACKAGE_H

#include <osg/Matrix>
#include <osg/Quat>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>

#include <sol/sol.hpp>

namespace LuaUtil
{
    using Vec2 = osg::Vec2f;
    using Vec3 = osg::Vec3f;
    using Vec4 = osg::Vec4f;

    // For performance reasons "Transform" is implemented as 2 types with the same interface.
    // Transform supports only composition, inversion, and applying to a 3d vector.
    struct TransformM
    {
        osg::Matrixf mM;
    };
    struct TransformQ
    {
        osg::Quat mQ;
    };

    inline TransformM asTransform(const osg::Matrixf& m)
    {
        return { m };
    }
    inline TransformQ asTransform(const osg::Quat& q)
    {
        return { q };
    }

    inline bool isTransform(const sol::object& obj)
    {
        return obj.is<TransformM>() || obj.is<TransformQ>();
    }

    sol::table initUtilPackage(lua_State* state);
}

#endif // COMPONENTS_LUA_UTILPACKAGE_H
