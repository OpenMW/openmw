#include "utilpackage.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <sstream>

#include <components/misc/color.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/mathutil.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/norm.hpp>

#include <MyGUI_StringUtility.h>

#include "luastate.hpp"
#include "util.hpp"

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
        float zero(const T& v)
        {
            return 0.f;
        }

        template <typename T>
        float one(const T& v)
        {
            return 1.f;
        }

        template <typename T, std::size_t I>
        float get(const T& v)
        {
            return v[I];
        }

        // Creates bindings for all possible permutations (repetition allowed) of x,y,z,w fields
        template <typename T>
        void addSwizzleFields(sol::usertype<T>& type)
        {
            // glm::vec* exposes the component count via a constexpr static length() method.
            constexpr int kN = static_cast<int>(T::length());

            // Generate mapping of swizzle characters to their getter functions
            constexpr auto components = []() {
                std::array<std::pair<char, float (*)(const T&)>, kN + 2> arr;

                // 0/1 Components
                arr[kN] = { '0', zero<T> };
                arr[kN + 1] = { '1', one<T> };

                // x,y,z,w components
                if constexpr (kN > 1)
                {
                    arr[0] = { 'x', get<T, 0> };
                    arr[1] = { 'y', get<T, 1> };
                }

                if constexpr (kN > 2)
                    arr[2] = { 'z', get<T, 2> };

                if constexpr (kN > 3)
                    arr[3] = { 'w', get<T, 3> };

                return arr;
            }();

            // Iterate over the permutations
            for (const auto& comp1 : components)
            {
                // Single component swizzle
                type[std::string{ comp1.first }] = sol::readonly_property([=](const T& v) { return comp1.second(v); });

                for (const auto& comp2 : components)
                {
                    // Two component swizzles
                    type[std::string{ comp1.first, comp2.first }]
                        = sol::readonly_property([=](const T& v) { return Vec2(comp1.second(v), comp2.second(v)); });

                    for (const auto& comp3 : components)
                    {
                        // Three component swizzles
                        type[std::string{ comp1.first, comp2.first, comp3.first }] = sol::readonly_property(
                            [=](const T& v) { return Vec3(comp1.second(v), comp2.second(v), comp3.second(v)); });

                        for (const auto& comp4 : components)
                        {
                            // Four component swizzles
                            type[std::string{ comp1.first, comp2.first, comp3.first, comp4.first }]
                                = sol::readonly_property([=](const T& v) {
                                      return Vec4(comp1.second(v), comp2.second(v), comp3.second(v), comp4.second(v));
                                  });
                        }
                    }
                }
            }
        }

        template <typename T>
        void addVectorMethods(sol::usertype<T>& vectorType)
        {
            constexpr int kN = static_cast<int>(T::length());
            vectorType[sol::meta_function::unary_minus] = [](const T& a) { return -a; };
            vectorType[sol::meta_function::addition] = [](const T& a, const T& b) { return a + b; };
            vectorType[sol::meta_function::subtraction] = [](const T& a, const T& b) { return a - b; };
            vectorType[sol::meta_function::equal_to] = [](const T& a, const T& b) { return a == b; };
            // Lua contract: v * v is the DOT PRODUCT (pinned by the openmw.util math conventions
            // integration test). glm's operator* on vectors is component-wise, so route through
            // glm::dot explicitly.
            vectorType[sol::meta_function::multiplication] = sol::overload(
                [](const T& a, float c) { return a * c; }, [](const T& a, const T& b) { return glm::dot(a, b); });
            vectorType[sol::meta_function::division] = [](const T& a, float c) { return a / c; };
            vectorType["dot"] = [](const T& a, const T& b) { return glm::dot(a, b); };
            vectorType["length"] = [](const T& v) { return glm::length(v); };
            vectorType["length2"] = [](const T& v) { return glm::length2(v); };
            vectorType["normalize"] = [](const T& v) {
                float len = glm::length(v);
                if (len == 0)
                    return std::make_tuple(T(0.0f), 0.f);
                else
                    return std::make_tuple(v * (1.f / len), len);
            };
            // glm's operator* / operator/ on two vectors is already component-wise.
            vectorType["emul"] = [](const T& a, const T& b) { return a * b; };
            vectorType["ediv"] = [](const T& a, const T& b) { return a / b; };
            vectorType[sol::meta_function::to_string] = [](const T& v) {
                std::stringstream ss;
                ss << std::setprecision(std::numeric_limits<typename T::value_type>::max_exponent10);
                ss << "(" << v[0];
                for (int i = 1; i < kN; ++i)
                    ss << ", " << v[i];
                ss << ")";
                return ss.str();
            };

            addSwizzleFields(vectorType);
        }
    }

    sol::table initUtilPackage(lua_State* state)
    {
        sol::state_view lua(state);
        sol::table util(lua, sol::create);

        // Lua bindings for Vec2
        util["vector2"] = [](float x, float y) { return Vec2(x, y); };
        sol::usertype<Vec2> vec2Type = lua.new_usertype<Vec2>("Vec2");
        addVectorMethods<Vec2>(vec2Type);
        vec2Type["rotate"] = [](const Vec2& v, float angle) {
            return Misc::Convert::toGlm(Misc::rotateVec2f(Misc::Convert::toOsg(v), angle));
        };

        // Lua bindings for Vec3
        util["vector3"] = [](float x, float y, float z) { return Vec3(x, y, z); };
        sol::usertype<Vec3> vec3Type = lua.new_usertype<Vec3>("Vec3");
        addVectorMethods<Vec3>(vec3Type);
        vec3Type[sol::meta_function::involution] = [](const Vec3& a, const Vec3& b) { return glm::cross(a, b); };
        vec3Type["cross"] = [](const Vec3& a, const Vec3& b) { return glm::cross(a, b); };

        // Lua bindings for Vec4
        util["vector4"] = [](float x, float y, float z, float w) { return Vec4(x, y, z, w); };
        sol::usertype<Vec4> vec4Type = lua.new_usertype<Vec4>("Vec4");
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
                table[toLuaIndex(i)] = vertices[i];
            return table;
        });
        boxType[sol::meta_function::equal_to] = [](const Box& a, const Box& b) { return a == b; };
        boxType[sol::meta_function::to_string] = [](const Box& b) {
            std::stringstream ss;
            ss << "Box{ ";
            ss << "center(" << b.mCenter.x << ", " << b.mCenter.y << ", " << b.mCenter.z << ") ";
            ss << "halfSize(" << b.mHalfSize.x << ", " << b.mHalfSize.y << ", " << b.mHalfSize.z << ")";
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
        colorType[sol::meta_function::equal_to] = [](const Misc::Color& a, const Misc::Color& b) { return a == b; };

        sol::table color(lua, sol::create);
        color["rgba"] = [](float r, float g, float b, float a) { return Misc::Color(r, g, b, a); };
        color["rgb"] = [](float r, float g, float b) { return Misc::Color(r, g, b, 1); };
        color["hex"] = [](std::string_view hex) { return Misc::Color::fromHex(hex); };
        color["commaString"] = [](std::string_view str) {
            auto wrongChars = std::count_if(
                str.begin(), str.end(), [](unsigned char c) { return !std::isdigit(c) && c != ' ' && c != ','; });

            if (wrongChars != 0)
            {
                throw std::runtime_error("Invalid comma-separated color: " + std::string(str));
            }

            std::vector<std::string> rgba;
            Misc::StringUtils::split(str, rgba, ",");
            if (rgba.size() != 3 && rgba.size() != 4)
            {
                throw std::runtime_error("Invalid comma-separated color: " + std::string(str));
            }

            if (rgba.size() == 3)
                rgba.push_back("255");

            return Misc::Color(MyGUI::utility::parseInt(rgba[0]) / 255.f, MyGUI::utility::parseInt(rgba[1]) / 255.f,
                MyGUI::utility::parseInt(rgba[2]) / 255.f, MyGUI::utility::parseInt(rgba[3]) / 255.f);
        };
        util["color"] = LuaUtil::makeReadOnly(color);

        // Lua bindings for Transform
        sol::usertype<TransformM> transMType = lua.new_usertype<TransformM>("TransformM");
        sol::usertype<TransformQ> transQType = lua.new_usertype<TransformQ>("TransformQ");
        sol::table transforms(lua, sol::create);
        util["transform"] = LuaUtil::makeReadOnly(transforms);

        // Lua transform composition contract (pinned by the openmw.util math conventions test):
        // (A * B) * v applies B first, then A. In glm's column-vector convention that means the
        // stored matrix is A * B (natural order), whereas the old osg row-vector code stored
        // B * A. Similarly osg::Quat::operator*(Quat) is Hamilton-reversed vs glm, so quaternion
        // composition operand order is also swapped from the previous implementation.
        transforms["identity"] = sol::make_object(lua, TransformQ{ glm::quat(1, 0, 0, 0) });
        transforms["move"]
            = sol::overload([](const Vec3& v) { return TransformM{ glm::translate(glm::mat4(1.0f), v) }; },
                [](float x, float y, float z) { return TransformM{ glm::translate(glm::mat4(1.0f), Vec3(x, y, z)) }; });
        transforms["scale"] = sol::overload([](const Vec3& v) { return TransformM{ glm::scale(glm::mat4(1.0f), v) }; },
            [](float x, float y, float z) { return TransformM{ glm::scale(glm::mat4(1.0f), Vec3(x, y, z)) }; });
        transforms["rotate"] = [](float angle, const Vec3& axis) {
            // Match osg::Quat::makeRotate: a near-zero axis gives the identity, where glm::normalize would give NaN.
            const float length = glm::length(axis);
            if (length < 1e-7f)
                return TransformQ{ glm::quat(1, 0, 0, 0) };
            return TransformQ{ glm::angleAxis(angle, axis / length) };
        };
        transforms["rotateX"] = [](float angle) { return TransformQ{ glm::angleAxis(angle, Vec3(-1, 0, 0)) }; };
        transforms["rotateY"] = [](float angle) { return TransformQ{ glm::angleAxis(angle, Vec3(0, -1, 0)) }; };
        transforms["rotateZ"] = [](float angle) { return TransformQ{ glm::angleAxis(angle, Vec3(0, 0, -1)) }; };

        transMType[sol::meta_function::multiplication]
            = sol::overload([](const TransformM& a, const Vec3& b) { return Vec3(a.mM * glm::vec4(b, 1.0f)); },
                [](const TransformM& a, const TransformM& b) { return TransformM{ a.mM * b.mM }; },
                [](const TransformM& a, const TransformQ& b) { return TransformM{ a.mM * glm::mat4_cast(b.mQ) }; });
        transMType[sol::meta_function::to_string] = [](const TransformM& m) {
            glm::vec3 scale{};
            glm::quat rotation{};
            glm::vec3 trans{};
            glm::vec3 skew{};
            glm::vec4 perspective{};
            glm::decompose(m.mM, scale, rotation, trans, skew, perspective);
            const float rotationAngle = glm::angle(rotation);
            const glm::vec3 rotationAxis = glm::axis(rotation);
            std::stringstream ss;
            ss << "TransformM{ ";
            if (glm::length2(trans) > 0)
                ss << "move(" << trans.x << ", " << trans.y << ", " << trans.z << ") ";
            if (rotationAngle != 0)
                ss << "rotation(angle=" << rotationAngle << ", axis=(" << rotationAxis.x << ", " << rotationAxis.y
                   << ", " << rotationAxis.z << ")) ";
            if (scale.x != 1 || scale.y != 1 || scale.z != 1)
                ss << "scale(" << scale.x << ", " << scale.y << ", " << scale.z << ") ";
            ss << "}";
            return ss.str();
        };
        transMType["apply"] = [](const TransformM& a, const Vec3& b) { return Vec3(a.mM * glm::vec4(b, 1.0f)); };
        transMType["inverse"] = [](const TransformM& m) {
            const float det = glm::determinant(m.mM);
            if (std::abs(det) <= std::numeric_limits<float>::epsilon())
                throw std::runtime_error("This Transform is not invertible");
            return TransformM{ glm::inverse(m.mM) };
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
                [](const TransformQ& a, const TransformQ& b) { return TransformQ{ a.mQ * b.mQ }; },
                [](const TransformQ& a, const TransformM& b) { return TransformM{ glm::mat4_cast(a.mQ) * b.mM }; });
        transQType[sol::meta_function::to_string] = [](const TransformQ& q) {
            const float angle = glm::angle(q.mQ);
            const glm::vec3 axis = glm::axis(q.mQ);
            std::stringstream ss;
            ss << "TransformQ{ rotation(angle=" << angle << ", axis=(" << axis.x << ", " << axis.y << ", " << axis.z
               << ")) }";
            return ss.str();
        };
        transQType["apply"] = [](const TransformQ& a, const Vec3& b) { return a.mQ * b; };
        transQType["inverse"] = [](const TransformQ& q) { return TransformQ{ glm::inverse(q.mQ) }; };
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

        sol::function luaUtilLoader = lua["loadInternalLib"]("util");
        sol::table utils = luaUtilLoader();
        for (const auto& [key, value] : utils)
            util[key.as<std::string>()] = value;

        // Utility functions
        util["makeReadOnly"] = [](const sol::table& tbl) { return makeReadOnly(tbl, /*strictIndex=*/false); };
        util["makeStrictReadOnly"] = [](const sol::table& tbl) { return makeReadOnly(tbl, /*strictIndex=*/true); };

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
            sol::state_view thisState(s);
            sol::load_result res = thisState.load(code, "", sol::load_mode::text);
            if (!res.valid())
                throw std::runtime_error("Lua error: " + res.get<std::string>());
            sol::function fn = res;
            sol::environment newEnv(thisState, sol::create, env);
            newEnv[sol::metatable_key][sol::meta_function::new_index] = env;
            sol::set_environment(newEnv, fn);
            return fn;
        };

        return util;
    }
}
