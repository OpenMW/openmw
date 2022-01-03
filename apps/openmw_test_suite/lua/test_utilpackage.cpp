#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/lua/luastate.hpp>
#include <components/lua/utilpackage.hpp>

#include "testing_util.hpp"

namespace
{
    using namespace testing;

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
        EXPECT_ERROR(lua.safe_script("v2, len = v.normalize()"), "value is not a valid userdata");  // checks that it doesn't segfault
        lua.safe_script("v2, len = v:normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 5);
        EXPECT_TRUE(get<bool>(lua, "v2 == util.vector2(3/5, 4/5)"));
        lua.safe_script("_, len = util.vector2(0, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 0);
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
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector3(3, 2, 1) * v"), 5*3 + 12*2 + 13*1);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.vector3(3, 2, 1):dot(v)"), 5*3 + 12*2 + 13*1);
        EXPECT_TRUE(get<bool>(lua, "util.vector3(1, 0, 0) ^ util.vector3(0, 1, 0) == util.vector3(0, 0, 1)"));
        EXPECT_ERROR(lua.safe_script("v2, len = util.vector3(3, 4, 0).normalize()"), "value is not a valid userdata");
        lua.safe_script("v2, len = util.vector3(3, 4, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 5);
        EXPECT_TRUE(get<bool>(lua, "v2 == util.vector3(3/5, 4/5, 0)"));
        lua.safe_script("_, len = util.vector3(0, 0, 0):normalize()");
        EXPECT_FLOAT_EQ(get<float>(lua, "len"), 0);
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
        EXPECT_THAT(getAsString(lua, "moveAndScale"), AllOf(StartsWith("TransformM{ move(6, 22, 18) scale(0.5, 1, 0.5) "), EndsWith(" }")));
        EXPECT_EQ(getAsString(lua, "T.identity"), "TransformM{ }");
        lua.safe_script("rx = T.rotateX(-math.pi / 2)");
        lua.safe_script("ry = T.rotateY(-math.pi / 2)");
        lua.safe_script("rz = T.rotateZ(-math.pi / 2)");
        EXPECT_LT(get<float>(lua, "(rx * v(1, 2, 3) - v(1, -3, 2)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(ry * v(1, 2, 3) - v(3, 2, -1)):length()"), 1e-6);
        EXPECT_LT(get<float>(lua, "(rz * v(1, 2, 3) - v(-2, 1, 3)):length()"), 1e-6);
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
        EXPECT_FLOAT_EQ(get<float>(lua, "v.x"), -0.5);
        EXPECT_FLOAT_EQ(get<float>(lua, "v.y"), 0.86602539);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.normalizeAngle(math.pi * 10 + 0.1)"), 0.1);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.clamp(0.1, 0, 1.5)"), 0.1);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.clamp(-0.1, 0, 1.5)"), 0);
        EXPECT_FLOAT_EQ(get<float>(lua, "util.clamp(2.1, 0, 1.5)"), 1.5);
        lua.safe_script("t = util.makeReadOnly({x = 1})");
        EXPECT_FLOAT_EQ(get<float>(lua, "t.x"), 1);
        EXPECT_ERROR(lua.safe_script("t.y = 2"), "userdata value");
    }

}
