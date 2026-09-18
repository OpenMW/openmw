#ifndef COMPONENTS_LUA_UTILPACKAGE_H
#define COMPONENTS_LUA_UTILPACKAGE_H

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <sol/sol.hpp>

namespace LuaUtil
{
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;

    // For performance reasons "Transform" is implemented as 2 types with the same interface.
    // Transform supports only composition, inversion, and applying to a 3d vector.
    struct TransformM
    {
        glm::mat4 mM;
    };
    struct TransformQ
    {
        glm::quat mQ;
    };

    inline TransformM asTransform(const glm::mat4& m)
    {
        return { m };
    }
    inline TransformQ asTransform(const glm::quat& q)
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
