#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/lua/luastate.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/testing/expecterror.hpp>

namespace
{
    using namespace testing;

    template <typename T>
    T get(sol::state& lua, const std::string& luaCode)
    {
        return lua.safe_script("return " + luaCode).get<T>();
    }

    std::string getAsString(sol::state& lua, std::string luaCode)
    {
        return LuaUtil::toString(lua.safe_script("return " + luaCode));
    }

    TEST(LuaUtilPackageTest, Vector2)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector2(3, 4)");
        EXPECT_FLOAT_EQ(get<float>(lua, "v.x"), 3);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.y"), 4);
        EXPECT_EQ(get<std::string>(lua, "tostring(v)"), "(3, 4)");
        EXPECT_FLOAT_EQ(get<float>(lua, "v:length()"), 5);
        EXPECT_FLOAT_EQ(get<float>(lua, "v:length2()"), 25);
        EXPECT_FALSE(get<bool>(lua, "util.vector2(1, 2) == util.vector2(1, 3)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector2(1, 2) + util.vector2(2, 5) == util.vector2(3, 7)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector2(1, 2) - util.vector2(2, 5) == -util.vector2(1, 3)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector2(1, 2) == util.vector2(2, 4) / 2"));
        EXPECT_TRUE(get<bool>(lua, "util.vector2(1, 2) * 2 == util.vector2(2, 4)"));
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector2(3, 2) * v"), 17);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector2(3, 2):dot(v)"), 17);
        EXPECT_ERROR(lua.safe_script("v2, len = v.normalize()"),
            "value is not a valid userdata"); // checks that it doesn't segfault
        lua.safe_script("v2, len = v:normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 5);
        EXPECT_TRUE(get<bool>(lua, "v2 == util.vector2(3/5, 4/5)"));
        lua.safe_script("_, len = util.vector2(0, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 0);
        lua.safe_script("ediv0 = util.vector2(1, 0):ediv(util.vector2(0, 0))");
        EXPECT_TRUE(get<bool>(lua, "ediv0.x == math.huge and ediv0.y ~= ediv0.y"));
        EXPECT_TRUE(get<bool>(lua, "util.vector2(1, 2):emul(util.vector2(3, 4)) == util.vector2(3, 8)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector2(4, 6):ediv(util.vector2(2, 3)) == util.vector2(2, 2)"));
        lua.safe_script("swizzle = util.vector2(1, 2)");
        EXPECT_TRUE(get<bool>(lua, "swizzle.xx == util.vector2(1, 1) and swizzle.yy == util.vector2(2, 2)"));
    }

    TEST(LuaUtilPackageTest, Vector3)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector3(5, 12, 13)");
        EXPECT_FLOAT_EQ(get<float>(lua, "v.x"), 5);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.y"), 12);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.z"), 13);
        EXPECT_EQ(get<std::string>(lua, "tostring(v)"), "(5, 12, 13)");
        EXPECT_EQ(getAsString(lua, "v"), "(5, 12, 13)");
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector3(4, 0, 3):length()"), 5);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector3(4, 0, 3):length2()"), 25);
        EXPECT_FALSE(get<bool>(lua, "util.vector3(1, 2, 3) == util.vector3(1, 3, 2)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 2, 3) + util.vector3(2, 5, 1) == util.vector3(3, 7, 4)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 2, 3) - util.vector3(2, 5, 1) == -util.vector3(1, 3, -2)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 2, 3) == util.vector3(2, 4, 6) / 2"));
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 2, 3) * 2 == util.vector3(2, 4, 6)"));
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector3(3, 2, 1) * v"), 5 * 3 + 12 * 2 + 13 * 1);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector3(3, 2, 1):dot(v)"), 5 * 3 + 12 * 2 + 13 * 1);
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 0, 0) ^ util.vector3(0, 1, 0) == util.vector3(0, 0, 1)"));
        EXPECT_ERROR(lua.safe_script("v2, len = util.vector3(3, 4, 0).normalize()"), "value is not a valid userdata");
        lua.safe_script("v2, len = util.vector3(3, 4, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 5);
        EXPECT_TRUE(get<bool>(lua, "v2 == util.vector3(3/5, 4/5, 0)"));
        lua.safe_script("_, len = util.vector3(0, 0, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 0);
        lua.safe_script("ediv0 = util.vector3(1, 1, 1):ediv(util.vector3(0, 0, 0))");
        EXPECT_TRUE(get<bool>(lua, "ediv0.z == math.huge"));
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 2, 3):emul(util.vector3(3, 4, 5)) == util.vector3(3, 8, 15)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector3(4, 6, 8):ediv(util.vector3(2, 3, 4)) == util.vector3(2, 2, 2)"));
        lua.safe_script("swizzle = util.vector3(1, 2, 3)");
        EXPECT_TRUE(get<bool>(lua, "swizzle.xxx == util.vector3(1, 1, 1)"));
        EXPECT_TRUE(get<bool>(lua, "swizzle.xyz == swizzle.zyx.zyx"));
    }

    TEST(LuaUtilPackageTest, Vector4)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector4(5, 12, 13, 15)");
        EXPECT_FLOAT_EQ(get<float>(lua, "v.x"), 5);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.y"), 12);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.z"), 13);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.w"), 15);
        EXPECT_EQ(get<std::string>(lua, "tostring(v)"), "(5, 12, 13, 15)");
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector4(4, 0, 0, 3):length()"), 5);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector4(4, 0, 0, 3):length2()"), 25);
        EXPECT_FALSE(get<bool>(lua, "util.vector4(1, 2, 3, 4) == util.vector4(1, 3, 2, 4)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector4(1, 2, 3, 4) + util.vector4(2, 5, 1, 2) == util.vector4(3, 7, 4, 6)"));
        EXPECT_TRUE(
            get<bool>(lua, "util.vector4(1, 2, 3, 4) - util.vector4(2, 5, 1, 7) == -util.vector4(1, 3, -2, 3)"));
        EXPECT_TRUE(get<bool>(lua, "util.vector4(1, 2, 3, 4) == util.vector4(2, 4, 6, 8) / 2"));
        EXPECT_TRUE(get<bool>(lua, "util.vector4(1, 2, 3, 4) * 2 == util.vector4(2, 4, 6, 8)"));
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector4(3, 2, 1, 4) * v"), 5 * 3 + 12 * 2 + 13 * 1 + 15 * 4);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector4(3, 2, 1, 4):dot(v)"), 5 * 3 + 12 * 2 + 13 * 1 + 15 * 4);
        lua.safe_script("v2, len = util.vector4(3, 0, 0, 4):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 5);
        EXPECT_TRUE(get<bool>(lua, "v2 == util.vector4(3/5, 0, 0, 4/5)"));
        lua.safe_script("_, len = util.vector4(0, 0, 0, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 0);
        lua.safe_script("ediv0 = util.vector4(1, 1, 1, -1):ediv(util.vector4(0, 0, 0, 0))");
        EXPECT_TRUE(get<bool>(lua, "ediv0.w == -math.huge"));
        EXPECT_TRUE(
            get<bool>(lua, "util.vector4(1, 2, 3, 4):emul(util.vector4(3, 4, 5, 6)) == util.vector4(3, 8, 15, 24)"));
        EXPECT_TRUE(
            get<bool>(lua, "util.vector4(4, 6, 8, 9):ediv(util.vector4(2, 3, 4, 3)) == util.vector4(2, 2, 2, 3)"));
        lua.safe_script("swizzle = util.vector4(1, 2, 3, 4)");
        EXPECT_TRUE(get<bool>(lua, "swizzle.wwww == util.vector4(4, 4, 4, 4)"));
        EXPECT_TRUE(get<bool>(lua, "swizzle.xyzw == util.vector4(1, 2, 3, 4)"));
        EXPECT_TRUE(get<bool>(lua, "swizzle.xyzw == swizzle.wzyx.wzyx"));
    }

    TEST(LuaUtilPackageTest, Color)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("brown = util.color.rgba(0.75, 0.25, 0, 1)");
        EXPECT_EQ(get<std::string>(lua, "tostring(brown)"), "(0.75, 0.25, 0, 1)");
        lua.safe_script("blue = util.color.rgb(0, 1, 0, 1)");
        EXPECT_EQ(get<std::string>(lua, "tostring(blue)"), "(0, 1, 0, 1)");
        lua.safe_script("red = util.color.hex('ff0000')");
        EXPECT_EQ(get<std::string>(lua, "tostring(red)"), "(1, 0, 0, 1)");
        lua.safe_script("green = util.color.hex('00FF00')");
        EXPECT_EQ(get<std::string>(lua, "tostring(green)"), "(0, 1, 0, 1)");
        lua.safe_script("darkRed = util.color.hex('a01112')");
        EXPECT_EQ(get<std::string>(lua, "darkRed:asHex()"), "a01112");
        EXPECT_TRUE(get<bool>(lua, "green:asRgba() == util.vector4(0, 1, 0, 1)"));
        EXPECT_TRUE(get<bool>(lua, "red:asRgb() == util.vector3(1, 0, 0)"));
    }

    TEST(LuaUtilPackageTest, Transform)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua["T"] = lua["util"]["transform"];
        lua["v"] = lua["util"]["vector3"];
        EXPECT_ERROR(lua.safe_script("T.identity = nil"), "attempt to index");
        EXPECT_EQ(getAsString(lua, "T.identity * v(3, 4, 5)"), "(3, 4, 5)");
        EXPECT_EQ(getAsString(lua, "T.move(1, 2, 3) * v(3, 4, 5)"), "(4, 6, 8)");
        EXPECT_EQ(getAsString(lua, "T.scale(1, -2, 3) * v(3, 4, 5)"), "(3, -8, 15)");
        EXPECT_EQ(getAsString(lua, "T.scale(v(1, 2, 3)) * v(3, 4, 5)"), "(3, 8, 15)");
        lua.safe_script("moveAndScale = T.move(v(1, 2, 3)) * T.scale(0.5, 1, 0.5) * T.move(10, 20, 30)");
        EXPECT_EQ(getAsString(lua, "moveAndScale * v(0, 0, 0)"), "(6, 22, 18)");
        EXPECT_EQ(getAsString(lua, "moveAndScale * v(300, 200, 100)"), "(156, 222, 68)");
        EXPECT_EQ(getAsString(lua, "moveAndScale:apply(v(0, 0, 0))"), "(6, 22, 18)");
        EXPECT_EQ(getAsString(lua, "moveAndScale:apply(v(300, 200, 100))"), "(156, 222, 68)");
        EXPECT_THAT(getAsString(lua, "moveAndScale"),
            AllOf(StartsWith("TransformM{ move(6, 22, 18) scale(0.5, 1, 0.5) "), EndsWith(" }")));
        EXPECT_EQ(getAsString(lua, "T.identity"), "TransformQ{ rotation(angle=0, axis=(0, 0, 1)) }");
        lua.safe_script("rx = T.rotateX(-math.pi / 2)");
        lua.safe_script("ry = T.rotateY(-math.pi / 2)");
        lua.safe_script("rz = T.rotateZ(-math.pi / 2)");
        EXPECT_LT(get<float>(lua, "(rx * v(1, 2, 3) - v(1, -3, 2)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(ry * v(1, 2, 3) - v(3, 2, -1)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(rz * v(1, 2, 3) - v(-2, 1, 3)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(rz:apply(v(1, 2, 3)) - v(-2, 1, 3)):length()"), 1e-6);
        lua.safe_script("rot = T.rotate(math.pi / 2, v(-1, -1, 0)) * T.rotateZ(math.pi / 4)");
        EXPECT_THAT(getAsString(lua, "rot"), HasSubstr("TransformQ"));
        EXPECT_LT(get<float>(lua, "(rot * v(1, 0, 0) - v(0, 0, 1)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(rot * rot:inverse() * v(1, 0, 0) - v(1, 0, 0)):length()"), 1e-6);
        lua.safe_script("rz_move_rx = rz * T.move(0, 3, 0) * rx");
        EXPECT_LT(get<float>(lua, "(rz_move_rx * v(1, 2, 3) - v(0, 1, 2)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(rz_move_rx:inverse() * v(0, 1, 2) - v(1, 2, 3)):length()"), 1e-6);
    }

    TEST(LuaUtilPackageTest, UtilityFunctions)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector2(1, 0):rotate(math.rad(120))");
        EXPECT_FLOAT_EQ(get<float>(lua, "v.x"), -0.5f);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.y"), 0.86602539f);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.normalizeAngle(math.pi * 10 + 0.1)"), 0.1f);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.clamp(0.1, 0, 1.5)"), 0.1f);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.clamp(-0.1, 0, 1.5)"), 0);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.clamp(2.1, 0, 1.5)"), 1.5f);
        lua.safe_script("t = util.makeReadOnly({x = 1})");
        EXPECT_FLOAT_EQ(get<float>(lua, "t.x"), 1);
        EXPECT_ERROR(lua.safe_script("t.y = 2"), "userdata value");
    }

}
