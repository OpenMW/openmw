#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/lua/inputactions.hpp>
#include <components/lua/scriptscontainer.hpp>
#include <components/testing/util.hpp>

namespace
{
    using namespace testing;
    using namespace TestingOpenMW;

    TEST(LuaInputActionsTest, MultiTree)
    {
        {
            LuaUtil::InputAction::MultiTree tree;
            auto a = tree.insert();
            auto b = tree.insert();
            auto c = tree.insert();
            auto d = tree.insert();
            EXPECT_TRUE(tree.multiEdge(c, { a, b }));
            EXPECT_TRUE(tree.multiEdge(a, { d }));
            EXPECT_FALSE(tree.multiEdge(d, { c }));
        }

        {
            LuaUtil::InputAction::MultiTree tree;
            auto a = tree.insert();
            auto b = tree.insert();
            auto c = tree.insert();
            EXPECT_TRUE(tree.multiEdge(b, { a }));
            EXPECT_TRUE(tree.multiEdge(c, { a, b }));
        }
    }

    TEST(LuaInputActionsTest, Registry)
    {
        sol::state lua;
        LuaUtil::InputAction::Registry registry;
        LuaUtil::InputAction::Info a({ "a", LuaUtil::InputAction::Type::Boolean, "test", "a_name", "a_description",
            sol::make_object(lua, false) });
        registry.insert(a);
        LuaUtil::InputAction::Info b({ "b", LuaUtil::InputAction::Type::Boolean, "test", "b_name", "b_description",
            sol::make_object(lua, false) });
        registry.insert(b);
        LuaUtil::Callback bindA({ lua.load("return function() return true end")(), sol::table(lua, sol::create) });
        LuaUtil::Callback bindBToA(
            { lua.load("return function(_, _, aValue) return aValue end")(), sol::table(lua, sol::create) });
        EXPECT_TRUE(registry.bind("a", bindA, {}));
        EXPECT_TRUE(registry.bind("b", bindBToA, { "a" }));
        registry.update(1.0);
        sol::object bValue = registry.valueOfType("b", LuaUtil::InputAction::Type::Boolean);
        EXPECT_TRUE(bValue.is<bool>());
        LuaUtil::Callback badA(
            { lua.load("return function() return 'not_a_bool' end")(), sol::table(lua, sol::create) });
        EXPECT_TRUE(registry.bind("a", badA, {}));
        testing::internal::CaptureStderr();
        registry.update(1.0);
        sol::object aValue = registry.valueOfType("a", LuaUtil::InputAction::Type::Boolean);
        EXPECT_TRUE(aValue.is<bool>());
        bValue = registry.valueOfType("b", LuaUtil::InputAction::Type::Boolean);
        EXPECT_TRUE(bValue.is<bool>() && bValue.as<bool>() == aValue.as<bool>());
    }
}
