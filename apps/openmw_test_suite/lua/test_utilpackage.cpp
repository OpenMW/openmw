#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/lua/utilpackage.hpp>

#include "testing_util.hpp"

namespace
{
    using namespace testing;

    TEST(LuaUtilPackageTest, Vector2)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector2(3, 4)");
        EXPECT_FLOAT_EQ(lua.safe_script("return v.x").get<float>(), 3);
        EXPECT_FLOAT_EQ(lua.safe_script("return v.y").get<float>(), 4);
        EXPECT_EQ(lua.safe_script("return tostring(v)").get<std::string>(), "(3, 4)");
        EXPECT_FLOAT_EQ(lua.safe_script("return v:length()").get<float>(), 5);
        EXPECT_FLOAT_EQ(lua.safe_script("return v:length2()").get<float>(), 25);
        EXPECT_FALSE(lua.safe_script("return util.vector2(1, 2) == util.vector2(1, 3)").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector2(1, 2) + util.vector2(2, 5) == util.vector2(3, 7)").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector2(1, 2) - util.vector2(2, 5) == -util.vector2(1, 3)").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector2(1, 2) == util.vector2(2, 4) / 2").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector2(1, 2) * 2 == util.vector2(2, 4)").get<bool>());
        EXPECT_FLOAT_EQ(lua.safe_script("return util.vector2(3, 2) * v").get<float>(), 17);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.vector2(3, 2):dot(v)").get<float>(), 17);
        EXPECT_ERROR(lua.safe_script("v2, len = v.normalize()"), "value is not a valid userdata");  // checks that it doesn't segfault
        lua.safe_script("v2, len = v:normalize()");
        EXPECT_FLOAT_EQ(lua.safe_script("return len").get<float>(), 5);
        EXPECT_TRUE(lua.safe_script("return v2 == util.vector2(3/5, 4/5)").get<bool>());
        lua.safe_script("_, len = util.vector2(0, 0):normalize()");
        EXPECT_FLOAT_EQ(lua.safe_script("return len").get<float>(), 0);
    }

    TEST(LuaUtilPackageTest, Vector3)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector3(5, 12, 13)");
        EXPECT_FLOAT_EQ(lua.safe_script("return v.x").get<float>(), 5);
        EXPECT_FLOAT_EQ(lua.safe_script("return v.y").get<float>(), 12);
        EXPECT_FLOAT_EQ(lua.safe_script("return v.z").get<float>(), 13);
        EXPECT_EQ(lua.safe_script("return tostring(v)").get<std::string>(), "(5, 12, 13)");
        EXPECT_FLOAT_EQ(lua.safe_script("return util.vector3(4, 0, 3):length()").get<float>(), 5);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.vector3(4, 0, 3):length2()").get<float>(), 25);
        EXPECT_FALSE(lua.safe_script("return util.vector3(1, 2, 3) == util.vector3(1, 3, 2)").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector3(1, 2, 3) + util.vector3(2, 5, 1) == util.vector3(3, 7, 4)").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector3(1, 2, 3) - util.vector3(2, 5, 1) == -util.vector3(1, 3, -2)").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector3(1, 2, 3) == util.vector3(2, 4, 6) / 2").get<bool>());
        EXPECT_TRUE(lua.safe_script("return util.vector3(1, 2, 3) * 2 == util.vector3(2, 4, 6)").get<bool>());
        EXPECT_FLOAT_EQ(lua.safe_script("return util.vector3(3, 2, 1) * v").get<float>(), 5*3 + 12*2 + 13*1);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.vector3(3, 2, 1):dot(v)").get<float>(), 5*3 + 12*2 + 13*1);
        EXPECT_TRUE(lua.safe_script("return util.vector3(1, 0, 0) ^ util.vector3(0, 1, 0) == util.vector3(0, 0, 1)").get<bool>());
        EXPECT_ERROR(lua.safe_script("v2, len = util.vector3(3, 4, 0).normalize()"), "value is not a valid userdata");
        lua.safe_script("v2, len = util.vector3(3, 4, 0):normalize()");
        EXPECT_FLOAT_EQ(lua.safe_script("return len").get<float>(), 5);
        EXPECT_TRUE(lua.safe_script("return v2 == util.vector3(3/5, 4/5, 0)").get<bool>());
        lua.safe_script("_, len = util.vector3(0, 0, 0):normalize()");
        EXPECT_FLOAT_EQ(lua.safe_script("return len").get<float>(), 0);
    }

    TEST(LuaUtilPackageTest, UtilityFunctions)
    {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
        lua["util"] = LuaUtil::initUtilPackage(lua);
        lua.safe_script("v = util.vector2(1, 0):rotate(math.rad(120))");
        EXPECT_FLOAT_EQ(lua.safe_script("return v.x").get<float>(), -0.5);
        EXPECT_FLOAT_EQ(lua.safe_script("return v.y").get<float>(), 0.86602539);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.normalizeAngle(math.pi * 10 + 0.1)").get<float>(), 0.1);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.clamp(0.1, 0, 1.5)").get<float>(), 0.1);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.clamp(-0.1, 0, 1.5)").get<float>(), 0);
        EXPECT_FLOAT_EQ(lua.safe_script("return util.clamp(2.1, 0, 1.5)").get<float>(), 1.5);
    }

}
