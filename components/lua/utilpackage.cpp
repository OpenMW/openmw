#include "utilpackage.hpp"

#include <algorithm>
#include <sstream>

#include <osg/Vec3f>

#include <components/misc/mathutil.hpp>

namespace sol
{
    template <>
    struct is_automagical<osg::Vec2f> : std::false_type {};

    template <>
    struct is_automagical<osg::Vec3f> : std::false_type {};
}

namespace LuaUtil
{

    sol::table initUtilPackage(sol::state& lua)
    {
        sol::table util(lua, sol::create);

        // TODO: Add bindings for osg::Matrix

        // Lua bindings for osg::Vec2f
        util["vector2"] = [](float x, float y) { return osg::Vec2f(x, y); };
        sol::usertype<osg::Vec2f> vec2Type = lua.new_usertype<osg::Vec2f>("Vec2");
        vec2Type["x"] = sol::readonly_property([](const osg::Vec2f& v) -> float { return v.x(); } );
        vec2Type["y"] = sol::readonly_property([](const osg::Vec2f& v) -> float { return v.y(); } );
        vec2Type[sol::meta_function::to_string] = [](const osg::Vec2f& v) {
            std::stringstream ss;
            ss << "(" << v.x() << ", " << v.y() << ")";
            return ss.str();
        };
        vec2Type[sol::meta_function::unary_minus] = [](const osg::Vec2f& a) { return -a; };
        vec2Type[sol::meta_function::addition] = [](const osg::Vec2f& a, const osg::Vec2f& b) { return a + b; };
        vec2Type[sol::meta_function::subtraction] = [](const osg::Vec2f& a, const osg::Vec2f& b) { return a - b; };
        vec2Type[sol::meta_function::equal_to] = [](const osg::Vec2f& a, const osg::Vec2f& b) { return a == b; };
        vec2Type[sol::meta_function::multiplication] = sol::overload(
            [](const osg::Vec2f& a, float c) { return a * c; },
            [](const osg::Vec2f& a, const osg::Vec2f& b) { return a * b; });
        vec2Type[sol::meta_function::division] = [](const osg::Vec2f& a, float c) { return a / c; };
        vec2Type["dot"] = [](const osg::Vec2f& a, const osg::Vec2f& b) { return a * b; };
        vec2Type["length"] = &osg::Vec2f::length;
        vec2Type["length2"] = &osg::Vec2f::length2;
        vec2Type["normalize"] = [](const osg::Vec2f& v) {
            float len = v.length();
            if (len == 0)
                return std::make_tuple(osg::Vec2f(), 0.f);
            else
                return std::make_tuple(v * (1.f / len), len);
        };
        vec2Type["rotate"] = &Misc::rotateVec2f;

        // Lua bindings for osg::Vec3f
        util["vector3"] = [](float x, float y, float z) { return osg::Vec3f(x, y, z); };
        sol::usertype<osg::Vec3f> vec3Type = lua.new_usertype<osg::Vec3f>("Vec3");
        vec3Type["x"] = sol::readonly_property([](const osg::Vec3f& v) -> float { return v.x(); } );
        vec3Type["y"] = sol::readonly_property([](const osg::Vec3f& v) -> float { return v.y(); } );
        vec3Type["z"] = sol::readonly_property([](const osg::Vec3f& v) -> float { return v.z(); } );
        vec3Type[sol::meta_function::to_string] = [](const osg::Vec3f& v) {
            std::stringstream ss;
            ss << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
            return ss.str();
        };
        vec3Type[sol::meta_function::unary_minus] = [](const osg::Vec3f& a) { return -a; };
        vec3Type[sol::meta_function::addition] = [](const osg::Vec3f& a, const osg::Vec3f& b) { return a + b; };
        vec3Type[sol::meta_function::subtraction] = [](const osg::Vec3f& a, const osg::Vec3f& b) { return a - b; };
        vec3Type[sol::meta_function::equal_to] = [](const osg::Vec3f& a, const osg::Vec3f& b) { return a == b; };
        vec3Type[sol::meta_function::multiplication] = sol::overload(
            [](const osg::Vec3f& a, float c) { return a * c; },
            [](const osg::Vec3f& a, const osg::Vec3f& b) { return a * b; });
        vec3Type[sol::meta_function::division] = [](const osg::Vec3f& a, float c) { return a / c; };
        vec3Type[sol::meta_function::involution] = [](const osg::Vec3f& a, const osg::Vec3f& b) { return a ^ b; };
        vec3Type["dot"] = [](const osg::Vec3f& a, const osg::Vec3f& b) { return a * b; };
        vec3Type["cross"] = [](const osg::Vec3f& a, const osg::Vec3f& b) { return a ^ b; };
        vec3Type["length"] = &osg::Vec3f::length;
        vec3Type["length2"] = &osg::Vec3f::length2;
        vec3Type["normalize"] = [](const osg::Vec3f& v) {
            float len = v.length();
            if (len == 0)
                return std::make_tuple(osg::Vec3f(), 0.f);
            else
                return std::make_tuple(v * (1.f / len), len);
        };

        // Utility functions
        util["clamp"] = [](float value, float from, float to) { return std::clamp(value, from, to); };
        // NOTE: `util["clamp"] = std::clamp<float>` causes error 'AddressSanitizer: stack-use-after-scope'
        util["normalizeAngle"] = &Misc::normalizeAngle;

        return util;
    }
    
}
