#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <osg/Matrixf>
#include <osg/Quat>
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/lua/serialization.hpp>
#include <components/lua/utilpackage.hpp>

#include <components/misc/color.hpp>
#include <components/misc/endianness.hpp>

#include <components/testing/expecterror.hpp>

namespace
{
    using namespace testing;

    TEST(LuaSerializationTest, Nil)
    {
        sol::state lua;
        EXPECT_EQ(LuaUtil::serialize(sol::nil), "");
        EXPECT_EQ(LuaUtil::deserialize(lua, ""), sol::nil);
    }

    TEST(LuaSerializationTest, Number)
    {
        sol::state lua;
        std::string serialized = LuaUtil::serialize(sol::make_object<double>(lua, 3.14));
        EXPECT_EQ(serialized.size(), 10); // version, type, 8 bytes value
        sol::object value = LuaUtil::deserialize(lua, serialized);
        ASSERT_TRUE(value.is<double>());
        EXPECT_DOUBLE_EQ(value.as<double>(), 3.14);
    }

    TEST(LuaSerializationTest, Boolean)
    {
        sol::state lua;
        {
            std::string serialized = LuaUtil::serialize(sol::make_object<bool>(lua, true));
            EXPECT_EQ(serialized.size(), 3); // version, type, 1 byte value
            sol::object value = LuaUtil::deserialize(lua, serialized);
            EXPECT_FALSE(value.is<double>());
            ASSERT_TRUE(value.is<bool>());
            EXPECT_TRUE(value.as<bool>());
        }
        {
            std::string serialized = LuaUtil::serialize(sol::make_object<bool>(lua, false));
            EXPECT_EQ(serialized.size(), 3); // version, type, 1 byte value
            sol::object value = LuaUtil::deserialize(lua, serialized);
            EXPECT_FALSE(value.is<double>());
            ASSERT_TRUE(value.is<bool>());
            EXPECT_FALSE(value.as<bool>());
        }
    }

    TEST(LuaSerializationTest, String)
    {
        sol::state lua;
        std::string_view emptyString = "";
        std::string_view shortString = "abc";
        std::string_view longString = "It is a string with more than 32 characters...........................";

        {
            std::string serialized = LuaUtil::serialize(sol::make_object<std::string_view>(lua, emptyString));
            EXPECT_EQ(serialized.size(), 2); // version, type
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<std::string>());
            EXPECT_EQ(value.as<std::string>(), emptyString);
        }
        {
            std::string serialized = LuaUtil::serialize(sol::make_object<std::string_view>(lua, shortString));
            EXPECT_EQ(serialized.size(), 2 + shortString.size()); // version, type, str data
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<std::string>());
            EXPECT_EQ(value.as<std::string>(), shortString);
        }
        {
            std::string serialized = LuaUtil::serialize(sol::make_object<std::string_view>(lua, longString));
            EXPECT_EQ(serialized.size(), 6 + longString.size()); // version, type, size, str data
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<std::string>());
            EXPECT_EQ(value.as<std::string>(), longString);
        }
    }

    TEST(LuaSerializationTest, Vector)
    {
        sol::state lua;
        osg::Vec2f vec2(1, 2);
        osg::Vec3f vec3(1, 2, 3);
        osg::Vec4f vec4(1, 2, 3, 4);

        {
            std::string serialized = LuaUtil::serialize(sol::make_object(lua, vec2));
            EXPECT_EQ(serialized.size(), 18); // version, type, 2x double
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<osg::Vec2f>());
            EXPECT_EQ(value.as<osg::Vec2f>(), vec2);
        }
        {
            std::string serialized = LuaUtil::serialize(sol::make_object(lua, vec3));
            EXPECT_EQ(serialized.size(), 26); // version, type, 3x double
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<osg::Vec3f>());
            EXPECT_EQ(value.as<osg::Vec3f>(), vec3);
        }
        {
            std::string serialized = LuaUtil::serialize(sol::make_object(lua, vec4));
            EXPECT_EQ(serialized.size(), 34); // version, type, 4x double
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<osg::Vec4f>());
            EXPECT_EQ(value.as<osg::Vec4f>(), vec4);
        }
    }

    TEST(LuaSerializationTest, Color)
    {
        sol::state lua;
        Misc::Color color(1, 1, 1, 1);

        {
            std::string serialized = LuaUtil::serialize(sol::make_object(lua, color));
            EXPECT_EQ(serialized.size(), 18); // version, type, 4x float
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<Misc::Color>());
            EXPECT_EQ(value.as<Misc::Color>(), color);
        }
    }

    TEST(LuaSerializationTest, Transform)
    {
        sol::state lua;
        osg::Matrixf matrix(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        LuaUtil::TransformM transM = LuaUtil::asTransform(matrix);
        osg::Quat quat(1, 2, 3, 4);
        LuaUtil::TransformQ transQ = LuaUtil::asTransform(quat);

        {
            std::string serialized = LuaUtil::serialize(sol::make_object(lua, transM));
            EXPECT_EQ(serialized.size(), 130); // version, type, 16x double
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<LuaUtil::TransformM>());
            EXPECT_EQ(value.as<LuaUtil::TransformM>().mM, transM.mM);
        }
        {
            std::string serialized = LuaUtil::serialize(sol::make_object(lua, transQ));
            EXPECT_EQ(serialized.size(), 34); // version, type, 4x double
            sol::object value = LuaUtil::deserialize(lua, serialized);
            ASSERT_TRUE(value.is<LuaUtil::TransformQ>());
            EXPECT_EQ(value.as<LuaUtil::TransformQ>().mQ, transQ.mQ);
        }
    }

    TEST(LuaSerializationTest, Table)
    {
        sol::state lua;
        sol::table table(lua, sol::create);
        table["aa"] = 1;
        table["ab"] = true;
        table["nested"] = sol::table(lua, sol::create);
        table["nested"]["aa"] = 2;
        table["nested"]["bb"] = "something";
        table["nested"][5] = -0.5;
        table["nested_empty"] = sol::table(lua, sol::create);
        table[1] = osg::Vec2f(1, 2);
        table[2] = osg::Vec2f(2, 1);

        std::string serialized = LuaUtil::serialize(table);
        EXPECT_EQ(serialized.size(), 139);
        sol::table resTable = LuaUtil::deserialize(lua, serialized);
        sol::table resReadonlyTable = LuaUtil::deserialize(lua, serialized, nullptr, true);

        for (auto t : { resTable, resReadonlyTable })
        {
            EXPECT_EQ(t.get<int>("aa"), 1);
            EXPECT_EQ(t.get<bool>("ab"), true);
            EXPECT_EQ(t.get<sol::table>("nested").get<int>("aa"), 2);
            EXPECT_EQ(t.get<sol::table>("nested").get<std::string>("bb"), "something");
            EXPECT_DOUBLE_EQ(t.get<sol::table>("nested").get<double>(5), -0.5);
            EXPECT_EQ(t.get<osg::Vec2f>(1), osg::Vec2f(1, 2));
            EXPECT_EQ(t.get<osg::Vec2f>(2), osg::Vec2f(2, 1));
        }

        lua["t"] = resTable;
        lua["ro_t"] = resReadonlyTable;
        EXPECT_NO_THROW(lua.safe_script("t.x = 5"));
        EXPECT_NO_THROW(lua.safe_script("t.nested.x = 5"));
        EXPECT_ERROR(lua.safe_script("ro_t.x = 5"), "userdata value");
        EXPECT_ERROR(lua.safe_script("ro_t.nested.x = 5"), "userdata value");
    }

    struct TestStruct1
    {
        double a, b;
    };
    struct TestStruct2
    {
        int a, b;
    };

    class TestSerializer final : public LuaUtil::UserdataSerializer
    {
        bool serialize(LuaUtil::BinaryData& out, const sol::userdata& data) const override
        {
            if (data.is<TestStruct1>())
            {
                TestStruct1 t = data.as<TestStruct1>();
                t.a = Misc::toLittleEndian(t.a);
                t.b = Misc::toLittleEndian(t.b);
                append(out, "ts1", &t, sizeof(t));
                return true;
            }
            if (data.is<TestStruct2>())
            {
                TestStruct2 t = data.as<TestStruct2>();
                t.a = Misc::toLittleEndian(t.a);
                t.b = Misc::toLittleEndian(t.b);
                append(out, "test_struct2", &t, sizeof(t));
                return true;
            }
            return false;
        }

        bool deserialize(std::string_view typeName, std::string_view binaryData, lua_State* lua) const override
        {
            if (typeName == "ts1")
            {
                if (sizeof(TestStruct1) != binaryData.size())
                    throw std::runtime_error(
                        "Incorrect binaryData.size() for TestStruct1: " + std::to_string(binaryData.size()));
                TestStruct1 t;
                std::memcpy(&t, binaryData.data(), sizeof(t));
                t.a = Misc::fromLittleEndian(t.a);
                t.b = Misc::fromLittleEndian(t.b);
                sol::stack::push<TestStruct1>(lua, t);
                return true;
            }
            if (typeName == "test_struct2")
            {
                if (sizeof(TestStruct2) != binaryData.size())
                    throw std::runtime_error(
                        "Incorrect binaryData.size() for TestStruct2: " + std::to_string(binaryData.size()));
                TestStruct2 t;
                std::memcpy(&t, binaryData.data(), sizeof(t));
                t.a = Misc::fromLittleEndian(t.a);
                t.b = Misc::fromLittleEndian(t.b);
                sol::stack::push<TestStruct2>(lua, t);
                return true;
            }
            return false;
        }
    };

    TEST(LuaSerializationTest, UserdataSerializer)
    {
        sol::state lua;
        sol::table table(lua, sol::create);
        table["x"] = TestStruct1{ 1.5, 2.5 };
        table["y"] = TestStruct2{ 4, 3 };
        TestSerializer serializer;

        EXPECT_ERROR(LuaUtil::serialize(table), "Value is not serializable.");
        std::string serialized = LuaUtil::serialize(table, &serializer);
        EXPECT_ERROR(LuaUtil::deserialize(lua, serialized), "Unknown type in serialized data:");
        sol::table res = LuaUtil::deserialize(lua, serialized, &serializer);

        TestStruct1 rx = res.get<TestStruct1>("x");
        TestStruct2 ry = res.get<TestStruct2>("y");
        EXPECT_EQ(rx.a, 1.5);
        EXPECT_EQ(rx.b, 2.5);
        EXPECT_EQ(ry.a, 4);
        EXPECT_EQ(ry.b, 3);
    }

}
