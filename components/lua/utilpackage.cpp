#include "utilpackage.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <sstream>

#include <components/misc/color.hpp>
#include <components/misc/mathutil.hpp>

#include "luastate.hpp"

#include "shapes/box.hpp"

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::Vec2> : std::false_type
    {
    };

    template <>
    struct is_automagical<LuaUtil::Vec3> : std::false_type
    {
    };

    template <>
    struct is_automagical<LuaUtil::Vec4> : std::false_type
    {
    };

    template <>
    struct is_automagical<Misc::Color> : std::false_type
    {
    };

    template <>
    struct is_automagical<LuaUtil::TransformM> : std::false_type
    {
    };

    template <>
    struct is_automagical<LuaUtil::TransformQ> : std::false_type
    {
    };

    template <>
    struct is_automagical<LuaUtil::Box> : std::false_type
    {
    };
}

namespace LuaUtil
{
    namespace
    {
        template <typename T>
        void addVectorMethods(sol::usertype<T>& vectorType)
        {
            vectorType[sol::meta_function::unary_minus] = [](const T& a) { return -a; };
            vectorType[sol::meta_function::addition] = [](const T& a, const T& b) { return a + b; };
            vectorType[sol::meta_function::subtraction] = [](const T& a, const T& b) { return a - b; };
            vectorType[sol::meta_function::equal_to] = [](const T& a, const T& b) { return a == b; };
            vectorType[sol::meta_function::multiplication] = sol::overload(
                [](const T& a, float c) { return a * c; }, [](const T& a, const T& b) { return a * b; });
            vectorType[sol::meta_function::division] = [](const T& a, float c) { return a / c; };
            vectorType["dot"] = [](const T& a, const T b) { return a * b; };
            vectorType["length"] = &T::length;
            vectorType["length2"] = &T::length2;
            vectorType["normalize"] = [](const T& v) {
                float len = v.length();
                if (len == 0)
                    return std::make_tuple(T(), 0.f);
                else
                    return std::make_tuple(v * (1.f / len), len);
            };
            vectorType["emul"] = [](const T& a, const T& b) {
                T result;
                for (int i = 0; i < T::num_components; ++i)
                    result[i] = a[i] * b[i];
                return result;
            };
            vectorType["ediv"] = [](const T& a, const T& b) {
                T result;
                for (int i = 0; i < T::num_components; ++i)
                    result[i] = a[i] / b[i];
                return result;
            };
            vectorType[sol::meta_function::to_string] = [](const T& v) {
                std::stringstream ss;
                ss << std::setprecision(std::numeric_limits<typename T::value_type>::max_exponent10);
                ss << "(" << v[0];
                for (int i = 1; i < T::num_components; ++i)
                    ss << ", " << v[i];
                ss << ")";
                return ss.str();
            };
        }
    }

    sol::table initUtilPackage(lua_State* L)
    {
        sol::state_view lua(L);
        sol::table util(lua, sol::create);

        // Lua bindings for Vec2
        util["vector2"] = [](float x, float y) { return Vec2(x, y); };
        sol::usertype<Vec2> vec2Type = lua.new_usertype<Vec2>("Vec2");
        vec2Type["x"] = sol::readonly_property([](const Vec2& v) -> float { return v.x(); });
        vec2Type["y"] = sol::readonly_property([](const Vec2& v) -> float { return v.y(); });
        addVectorMethods<Vec2>(vec2Type);
        vec2Type["rotate"] = &Misc::rotateVec2f;

        // Lua bindings for Vec3
        util["vector3"] = [](float x, float y, float z) { return Vec3(x, y, z); };
        sol::usertype<Vec3> vec3Type = lua.new_usertype<Vec3>("Vec3");
        vec3Type["x"] = sol::readonly_property([](const Vec3& v) -> float { return v.x(); });
        vec3Type["y"] = sol::readonly_property([](const Vec3& v) -> float { return v.y(); });
        vec3Type["z"] = sol::readonly_property([](const Vec3& v) -> float { return v.z(); });
        addVectorMethods<Vec3>(vec3Type);
        vec3Type[sol::meta_function::involution] = [](const Vec3& a, const Vec3& b) { return a ^ b; };
        vec3Type["cross"] = [](const Vec3& a, const Vec3& b) { return a ^ b; };

        // Lua bindings for Vec4
        util["vector4"] = [](float x, float y, float z, float w) { return Vec4(x, y, z, w); };
        sol::usertype<Vec4> vec4Type = lua.new_usertype<Vec4>("Vec4");
        vec4Type["x"] = sol::readonly_property([](const Vec4& v) -> float { return v.x(); });
        vec4Type["y"] = sol::readonly_property([](const Vec4& v) -> float { return v.y(); });
        vec4Type["z"] = sol::readonly_property([](const Vec4& v) -> float { return v.z(); });
        vec4Type["w"] = sol::readonly_property([](const Vec4& v) -> float { return v.w(); });
        addVectorMethods<Vec4>(vec4Type);

        // Lua bindings for Box
        util["box"] = sol::overload([](const Vec3& center, const Vec3& halfSize) { return Box(center, halfSize); },
            [](const TransformM& transform) { return Box(transform.mM); },
            [](const TransformQ& transform) { return Box(Vec3(), Vec3(1, 1, 1), transform.mQ); });
        sol::usertype<Box> boxType = lua.new_usertype<Box>("Box");
        boxType["center"] = sol::readonly_property([](const Box& b) { return b.mCenter; });
        boxType["halfSize"] = sol::readonly_property([](const Box& b) { return b.mHalfSize; });
        boxType["transform"] = sol::readonly_property([](const Box& b) { return TransformM{ b.asTransform() }; });
        boxType["vertices"] = sol::readonly_property([lua](const Box& b) {
            sol::table table(lua, sol::create);
            const auto vertices = b.vertices();
            for (size_t i = 0; i < vertices.size(); ++i)
                table[i + 1] = vertices[i];
            return table;
        });
        boxType[sol::meta_function::equal_to] = [](const Box& a, const Box& b) { return a == b; };
        boxType[sol::meta_function::to_string] = [](const Box& b) {
            std::stringstream ss;
            ss << "Box{ ";
            ss << "center(" << b.mCenter.x() << ", " << b.mCenter.y() << ", " << b.mCenter.z() << ") ";
            ss << "halfSize(" << b.mHalfSize.x() << ", " << b.mHalfSize.y() << ", " << b.mHalfSize.z() << ")";
            ss << " }";
            return ss.str();
        };

        // Lua bindings for Color
        sol::usertype<Misc::Color> colorType = lua.new_usertype<Misc::Color>("Color");
        colorType["r"] = sol::readonly_property([](const Misc::Color& c) { return c.r(); });
        colorType["g"] = sol::readonly_property([](const Misc::Color& c) { return c.g(); });
        colorType["b"] = sol::readonly_property([](const Misc::Color& c) { return c.b(); });
        colorType["a"] = sol::readonly_property([](const Misc::Color& c) { return c.a(); });
        colorType[sol::meta_function::to_string] = [](const Misc::Color& c) { return c.toString(); };
        colorType["asRgba"] = [](const Misc::Color& c) { return Vec4(c.r(), c.g(), c.b(), c.a()); };
        colorType["asRgb"] = [](const Misc::Color& c) { return Vec3(c.r(), c.g(), c.b()); };
        colorType["asHex"] = [](const Misc::Color& c) { return c.toHex(); };

        sol::table color(lua, sol::create);
        color["rgba"] = [](float r, float g, float b, float a) { return Misc::Color(r, g, b, a); };
        color["rgb"] = [](float r, float g, float b) { return Misc::Color(r, g, b, 1); };
        color["hex"] = [](std::string_view hex) { return Misc::Color::fromHex(hex); };
        util["color"] = LuaUtil::makeReadOnly(color);

        // Lua bindings for Transform
        sol::usertype<TransformM> transMType = lua.new_usertype<TransformM>("TransformM");
        sol::usertype<TransformQ> transQType = lua.new_usertype<TransformQ>("TransformQ");
        sol::table transforms(lua, sol::create);
        util["transform"] = LuaUtil::makeReadOnly(transforms);

        transforms["identity"] = sol::make_object(lua, TransformQ{ osg::Quat() });
        transforms["move"] = sol::overload([](const Vec3& v) { return TransformM{ osg::Matrixf::translate(v) }; },
            [](float x, float y, float z) { return TransformM{ osg::Matrixf::translate(x, y, z) }; });
        transforms["scale"] = sol::overload([](const Vec3& v) { return TransformM{ osg::Matrixf::scale(v) }; },
            [](float x, float y, float z) { return TransformM{ osg::Matrixf::scale(x, y, z) }; });
        transforms["rotate"] = [](float angle, const Vec3& axis) { return TransformQ{ osg::Quat(angle, axis) }; };
        transforms["rotateX"] = [](float angle) { return TransformQ{ osg::Quat(angle, Vec3(-1, 0, 0)) }; };
        transforms["rotateY"] = [](float angle) { return TransformQ{ osg::Quat(angle, Vec3(0, -1, 0)) }; };
        transforms["rotateZ"] = [](float angle) { return TransformQ{ osg::Quat(angle, Vec3(0, 0, -1)) }; };

        transMType[sol::meta_function::multiplication]
            = sol::overload([](const TransformM& a, const Vec3& b) { return a.mM.preMult(b); },
                [](const TransformM& a, const TransformM& b) { return TransformM{ b.mM * a.mM }; },
                [](const TransformM& a, const TransformQ& b) {
                    TransformM res{ a.mM };
                    res.mM.preMultRotate(b.mQ);
                    return res;
                });
        transMType[sol::meta_function::to_string] = [](const TransformM& m) {
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
                ss << "rotation(angle=" << rot_angle << ", axis=(" << rot_axis.x() << ", " << rot_axis.y() << ", "
                   << rot_axis.z() << ")) ";
            if (scale.x() != 1 || scale.y() != 1 || scale.z() != 1)
                ss << "scale(" << scale.x() << ", " << scale.y() << ", " << scale.z() << ") ";
            if (so_angle != 0)
                ss << "rotation(angle=" << so_angle << ", axis=(" << so_axis.x() << ", " << so_axis.y() << ", "
                   << so_axis.z() << ")) ";
            ss << "}";
            return ss.str();
        };
        transMType["apply"] = [](const TransformM& a, const Vec3& b) { return a.mM.preMult(b); },
        transMType["inverse"] = [](const TransformM& m) {
            TransformM res;
            if (!res.mM.invert_4x3(m.mM))
                throw std::runtime_error("This Transform is not invertible");
            return res;
        };
        transMType["getYaw"] = [](const TransformM& m) {
            osg::Vec3f angles = Misc::toEulerAnglesXZ(m.mM);
            return angles.z();
        };
        transMType["getPitch"] = [](const TransformM& m) {
            osg::Vec3f angles = Misc::toEulerAnglesXZ(m.mM);
            return angles.x();
        };
        transMType["getAnglesXZ"] = [](const TransformM& m) {
            osg::Vec3f angles = Misc::toEulerAnglesXZ(m.mM);
            return std::make_tuple(angles.x(), angles.z());
        };
        transMType["getAnglesZYX"] = [](const TransformM& m) {
            osg::Vec3f angles = Misc::toEulerAnglesZYX(m.mM);
            return std::make_tuple(angles.z(), angles.y(), angles.x());
        };

        transQType[sol::meta_function::multiplication]
            = sol::overload([](const TransformQ& a, const Vec3& b) { return a.mQ * b; },
                [](const TransformQ& a, const TransformQ& b) { return TransformQ{ b.mQ * a.mQ }; },
                [](const TransformQ& a, const TransformM& b) {
                    TransformM res{ b };
                    res.mM.postMultRotate(a.mQ);
                    return res;
                });
        transQType[sol::meta_function::to_string] = [](const TransformQ& q) {
            osg::Quat::value_type angle;
            osg::Vec3f axis;
            q.mQ.getRotate(angle, axis);
            std::stringstream ss;
            ss << "TransformQ{ rotation(angle=" << angle << ", axis=(" << axis.x() << ", " << axis.y() << ", "
               << axis.z() << ")) }";
            return ss.str();
        };
        transQType["apply"] = [](const TransformQ& a, const Vec3& b) { return a.mQ * b; },
        transQType["inverse"] = [](const TransformQ& q) { return TransformQ{ q.mQ.inverse() }; };
        transQType["getYaw"] = [](const TransformQ& q) {
            osg::Vec3f angles = Misc::toEulerAnglesXZ(q.mQ);
            return angles.z();
        };
        transQType["getPitch"] = [](const TransformQ& q) {
            osg::Vec3f angles = Misc::toEulerAnglesXZ(q.mQ);
            return angles.x();
        };
        transQType["getAnglesXZ"] = [](const TransformQ& q) {
            osg::Vec3f angles = Misc::toEulerAnglesXZ(q.mQ);
            return std::make_tuple(angles.x(), angles.z());
        };
        transQType["getAnglesZYX"] = [](const TransformQ& q) {
            osg::Vec3f angles = Misc::toEulerAnglesZYX(q.mQ);
            return std::make_tuple(angles.z(), angles.y(), angles.x());
        };

        // Utility functions
        util["clamp"] = [](double value, double from, double to) { return std::clamp(value, from, to); };
        // NOTE: `util["clamp"] = std::clamp<float>` causes error 'AddressSanitizer: stack-use-after-scope'
        util["normalizeAngle"] = &Misc::normalizeAngle;
        util["makeReadOnly"] = [](const sol::table& tbl) { return makeReadOnly(tbl, /*strictIndex=*/false); };
        util["makeStrictReadOnly"] = [](const sol::table& tbl) { return makeReadOnly(tbl, /*strictIndex=*/true); };
        util["remap"] = [](double value, double min, double max, double newMin, double newMax) {
            return newMin + (value - min) * (newMax - newMin) / (max - min);
        };
        util["round"] = [](double value) { return round(value); };

        if (lua["bit32"] != sol::nil)
        {
            sol::table bit = lua["bit32"];
            util["bitOr"] = bit["bor"];
            util["bitAnd"] = bit["band"];
            util["bitXor"] = bit["bxor"];
            util["bitNot"] = bit["bnot"];
        }
        else
        {
            util["bitOr"] = [](unsigned a, sol::variadic_args va) {
                for (const auto& v : va)
                    a |= cast<unsigned>(v);
                return a;
            };
            util["bitAnd"] = [](unsigned a, sol::variadic_args va) {
                for (const auto& v : va)
                    a &= cast<unsigned>(v);
                return a;
            };
            util["bitXor"] = [](unsigned a, sol::variadic_args va) {
                for (const auto& v : va)
                    a ^= cast<unsigned>(v);
                return a;
            };
            util["bitNot"] = [](unsigned a) { return ~a; };
        }

        util["loadCode"] = [](const std::string& code, const sol::table& env, sol::this_state s) {
            sol::state_view lua(s);
            sol::load_result res = lua.load(code, "", sol::load_mode::text);
            if (!res.valid())
                throw std::runtime_error("Lua error: " + res.get<std::string>());
            sol::function fn = res;
            sol::environment newEnv(lua, sol::create, env);
            newEnv[sol::metatable_key][sol::meta_function::new_index] = env;
            sol::set_environment(newEnv, fn);
            return fn;
        };

        return util;
    }

}
