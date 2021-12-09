#include "utilpackage.hpp"

#include <algorithm>
#include <sstream>

#include <components/misc/mathutil.hpp>

#include "luastate.hpp"

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::Vec2> : std::false_type {};

    template <>
    struct is_automagical<LuaUtil::Vec3> : std::false_type {};

    template <>
    struct is_automagical<LuaUtil::TransformM> : std::false_type {};

    template <>
    struct is_automagical<LuaUtil::TransformQ> : std::false_type {};
}

namespace LuaUtil
{

    sol::table initUtilPackage(sol::state& lua)
    {
        sol::table util(lua, sol::create);

        // Lua bindings for Vec2
        util["vector2"] = [](float x, float y) { return Vec2(x, y); };
        sol::usertype<Vec2> vec2Type = lua.new_usertype<Vec2>("Vec2");
        vec2Type["x"] = sol::readonly_property([](const Vec2& v) -> float { return v.x(); } );
        vec2Type["y"] = sol::readonly_property([](const Vec2& v) -> float { return v.y(); } );
        vec2Type[sol::meta_function::to_string] = [](const Vec2& v) {
            std::stringstream ss;
            ss << "(" << v.x() << ", " << v.y() << ")";
            return ss.str();
        };
        vec2Type[sol::meta_function::unary_minus] = [](const Vec2& a) { return -a; };
        vec2Type[sol::meta_function::addition] = [](const Vec2& a, const Vec2& b) { return a + b; };
        vec2Type[sol::meta_function::subtraction] = [](const Vec2& a, const Vec2& b) { return a - b; };
        vec2Type[sol::meta_function::equal_to] = [](const Vec2& a, const Vec2& b) { return a == b; };
        vec2Type[sol::meta_function::multiplication] = sol::overload(
            [](const Vec2& a, float c) { return a * c; },
            [](const Vec2& a, const Vec2& b) { return a * b; });
        vec2Type[sol::meta_function::division] = [](const Vec2& a, float c) { return a / c; };
        vec2Type["dot"] = [](const Vec2& a, const Vec2& b) { return a * b; };
        vec2Type["length"] = &Vec2::length;
        vec2Type["length2"] = &Vec2::length2;
        vec2Type["normalize"] = [](const Vec2& v) {
            float len = v.length();
            if (len == 0)
                return std::make_tuple(Vec2(), 0.f);
            else
                return std::make_tuple(v * (1.f / len), len);
        };
        vec2Type["rotate"] = &Misc::rotateVec2f;

        // Lua bindings for Vec3
        util["vector3"] = [](float x, float y, float z) { return Vec3(x, y, z); };
        sol::usertype<Vec3> vec3Type = lua.new_usertype<Vec3>("Vec3");
        vec3Type["x"] = sol::readonly_property([](const Vec3& v) -> float { return v.x(); } );
        vec3Type["y"] = sol::readonly_property([](const Vec3& v) -> float { return v.y(); } );
        vec3Type["z"] = sol::readonly_property([](const Vec3& v) -> float { return v.z(); } );
        vec3Type[sol::meta_function::to_string] = [](const Vec3& v) {
            std::stringstream ss;
            ss << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
            return ss.str();
        };
        vec3Type[sol::meta_function::unary_minus] = [](const Vec3& a) { return -a; };
        vec3Type[sol::meta_function::addition] = [](const Vec3& a, const Vec3& b) { return a + b; };
        vec3Type[sol::meta_function::subtraction] = [](const Vec3& a, const Vec3& b) { return a - b; };
        vec3Type[sol::meta_function::equal_to] = [](const Vec3& a, const Vec3& b) { return a == b; };
        vec3Type[sol::meta_function::multiplication] = sol::overload(
            [](const Vec3& a, float c) { return a * c; },
            [](const Vec3& a, const Vec3& b) { return a * b; });
        vec3Type[sol::meta_function::division] = [](const Vec3& a, float c) { return a / c; };
        vec3Type[sol::meta_function::involution] = [](const Vec3& a, const Vec3& b) { return a ^ b; };
        vec3Type["dot"] = [](const Vec3& a, const Vec3& b) { return a * b; };
        vec3Type["cross"] = [](const Vec3& a, const Vec3& b) { return a ^ b; };
        vec3Type["length"] = &Vec3::length;
        vec3Type["length2"] = &Vec3::length2;
        vec3Type["normalize"] = [](const Vec3& v) {
            float len = v.length();
            if (len == 0)
                return std::make_tuple(Vec3(), 0.f);
            else
                return std::make_tuple(v * (1.f / len), len);
        };

        // Lua bindings for Transform
        sol::usertype<TransformM> transMType = lua.new_usertype<TransformM>("TransformM");
        sol::usertype<TransformQ> transQType = lua.new_usertype<TransformQ>("TransformQ");
        sol::table transforms(lua, sol::create);
        util["transform"] = LuaUtil::makeReadOnly(transforms);

        transforms["identity"] = sol::make_object(lua, TransformM{osg::Matrixf::identity()});
        transforms["move"] = sol::overload(
            [](const Vec3& v) { return TransformM{osg::Matrixf::translate(v)}; },
            [](float x, float y, float z) { return TransformM{osg::Matrixf::translate(x, y, z)}; });
        transforms["scale"] = sol::overload(
            [](const Vec3& v) { return TransformM{osg::Matrixf::scale(v)}; },
            [](float x, float y, float z) { return TransformM{osg::Matrixf::scale(x, y, z)}; });
        transforms["rotate"] = [](float angle, const Vec3& axis) { return TransformQ{osg::Quat(angle, axis)}; };
        transforms["rotateX"] = [](float angle) { return TransformQ{osg::Quat(angle, Vec3(-1, 0, 0))}; };
        transforms["rotateY"] = [](float angle) { return TransformQ{osg::Quat(angle, Vec3(0, -1, 0))}; };
        transforms["rotateZ"] = [](float angle) { return TransformQ{osg::Quat(angle, Vec3(0, 0, -1))}; };

        transMType[sol::meta_function::multiplication] = sol::overload(
            [](const TransformM& a, const Vec3& b) { return a.mM.preMult(b); },
            [](const TransformM& a, const TransformM& b) { return TransformM{b.mM * a.mM}; },
            [](const TransformM& a, const TransformQ& b)
            {
                TransformM res{a.mM};
                res.mM.preMultRotate(b.mQ);
                return res;
            });
        transMType[sol::meta_function::to_string] = [](const TransformM& m)
        {
            osg::Vec3f trans, scale;
            osg::Quat rotation, so;
            m.mM.decompose(trans, rotation, scale, so);
            osg::Quat::value_type rot_angle, so_angle;
            osg::Vec3f rot_axis, so_axis;
            rotation.getRotate(rot_angle, rot_axis);
            so.getRotate(so_angle, so_axis);
            std::stringstream ss;
            ss << "TransformM{ ";
            if (trans.length2() > 0)
                ss << "move(" << trans.x() << ", " << trans.y() << ", " << trans.z() << ") ";
            if (rot_angle != 0)
                ss << "rotation(angle=" << rot_angle << ", axis=("
                    << rot_axis.x() << ", " << rot_axis.y() << ", " << rot_axis.z() << ")) ";
            if (scale.x() != 1 || scale.y() != 1 || scale.z() != 1)
                ss << "scale(" << scale.x() << ", " << scale.y() << ", " << scale.z() << ") ";
            if (so_angle != 0)
                ss << "rotation(angle=" << so_angle << ", axis=("
                    << so_axis.x() << ", " << so_axis.y() << ", " << so_axis.z() << ")) ";
            ss << "}";
            return ss.str();
        };
        transMType["inverse"] = [](const TransformM& m)
        {
            TransformM res;
            if (!res.mM.invert_4x3(m.mM))
                throw std::runtime_error("This Transform is not invertible");
            return res;
        };

        transQType[sol::meta_function::multiplication] = sol::overload(
            [](const TransformQ& a, const Vec3& b) { return a.mQ * b; },
            [](const TransformQ& a, const TransformQ& b) { return TransformQ{b.mQ * a.mQ}; },
            [](const TransformQ& a, const TransformM& b)
            {
                TransformM res{b};
                res.mM.postMultRotate(a.mQ);
                return res;
            });
        transQType[sol::meta_function::to_string] = [](const TransformQ& q)
        {
            osg::Quat::value_type angle;
            osg::Vec3f axis;
            q.mQ.getRotate(angle, axis);
            std::stringstream ss;
            ss << "TransformQ{ rotation(angle=" << angle << ", axis=("
                << axis.x() << ", " << axis.y() << ", " << axis.z() << ")) }";
            return ss.str();
        };
        transQType["inverse"] = [](const TransformQ& q) { return TransformQ{q.mQ.inverse()}; };

        // Utility functions
        util["clamp"] = [](float value, float from, float to) { return std::clamp(value, from, to); };
        // NOTE: `util["clamp"] = std::clamp<float>` causes error 'AddressSanitizer: stack-use-after-scope'
        util["normalizeAngle"] = &Misc::normalizeAngle;

        return util;
    }
    
}
