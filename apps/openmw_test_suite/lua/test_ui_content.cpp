#include <gtest/gtest.h>
#include <sol/sol.hpp>

#include <components/lua_ui/content.hpp>

namespace
{
    using namespace testing;

    sol::state state;

    sol::table makeTable()
    {
        return sol::table(state, sol::create);
    }

    sol::table makeTable(std::string name)
    {
        auto result = makeTable();
        result["name"] = name;
        return result;
    }

    TEST(LuaUiContentTest, Create)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::Content content(table);
        EXPECT_EQ(content.size(), 3);
    }

    TEST(LuaUiContentTest, CreateWithHole)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table[4] = makeTable();
        EXPECT_ANY_THROW(LuaUi::Content content(table));
    }

    TEST(LuaUiContentTest, WrongType)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add("a");
        table.add(makeTable());
        EXPECT_ANY_THROW(LuaUi::Content content(table));
    }

    TEST(LuaUiContentTest, NameAccess)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable("a"));
        LuaUi::Content content(table);
        EXPECT_NO_THROW(content.at("a"));
        content.remove("a");
        content.assign(content.size(), makeTable("b"));
        content.assign("b", makeTable());
        EXPECT_ANY_THROW(content.at("b"));
        EXPECT_EQ(content.size(), 2);
        content.assign(content.size(), makeTable("c"));
        content.assign(content.size(), makeTable("c"));
        content.remove("c");
        EXPECT_ANY_THROW(content.at("c"));
    }

    TEST(LuaUiContentTest, IndexOf)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::Content content(table);
        auto child = makeTable();
        content.assign(2, child);
        EXPECT_EQ(content.indexOf(child), 2);
        EXPECT_EQ(content.indexOf(makeTable()), content.size());
    }

    TEST(LuaUiContentTest, BoundsChecks)
    {
        auto table = makeTable();
        LuaUi::Content content(table);
        EXPECT_ANY_THROW(content.at(0));
        content.assign(content.size(), makeTable());
        content.assign(content.size(), makeTable());
        content.assign(content.size(), makeTable());
        EXPECT_ANY_THROW(content.at(3));
        EXPECT_ANY_THROW(content.remove(3));
        EXPECT_NO_THROW(content.remove(1));
        EXPECT_NO_THROW(content.at(1));
        EXPECT_EQ(content.size(), 2);
    }
}
