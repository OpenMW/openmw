#include <gtest/gtest.h>
#include <sol/sol.hpp>

#include <components/lua/luastate.hpp>
#include <components/lua_ui/content.hpp>

namespace
{
    using namespace testing;

    struct LuaUiContentTest : Test
    {
        LuaUtil::LuaState mLuaState{ nullptr, nullptr };
        sol::state_view mSol;
        sol::protected_function mNew;
        LuaUiContentTest()
            : mSol(mLuaState.sol())
            , mNew(LuaUi::Content::makeFactory(mSol))
        {
            mSol.open_libraries(sol::lib::base, sol::lib::table);
        }

        LuaUi::Content::View makeContent(sol::table source)
        {
            auto result = mNew.call(source);
            if (result.get_type() != sol::type::table)
                throw std::logic_error("Expected table");
            return LuaUi::Content::View(result.get<sol::table>());
        }

        sol::table makeTable() { return sol::table(mSol, sol::create); }

        sol::table makeTable(std::string name)
        {
            auto result = makeTable();
            result["name"] = name;
            return result;
        }
    };

    TEST_F(LuaUiContentTest, Create)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::Content::View content = makeContent(table);
        EXPECT_EQ(content.size(), 3);
    }

    TEST_F(LuaUiContentTest, Insert)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::Content::View content = makeContent(table);
        content.insert(2, makeTable("inserted"));
        EXPECT_EQ(content.size(), 4);
        auto inserted = content.at("inserted");
        auto index = content.indexOf(inserted);
        EXPECT_TRUE(index.has_value());
        EXPECT_EQ(index.value(), 2);
    }

    TEST_F(LuaUiContentTest, MakeHole)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::Content::View content = makeContent(table);
        sol::table t = makeTable();
        EXPECT_ANY_THROW(content.assign(3, t));
    }

    TEST_F(LuaUiContentTest, WrongType)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add("a");
        table.add(makeTable());
        EXPECT_ANY_THROW(makeContent(table));
    }

    TEST_F(LuaUiContentTest, NameAccess)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable("a"));
        LuaUi::Content::View content = makeContent(table);
        EXPECT_NO_THROW(content.at("a"));
        content.remove("a");
        EXPECT_EQ(content.size(), 1);
        content.assign(content.size(), makeTable("b"));
        content.assign("b", makeTable());
        EXPECT_ANY_THROW(content.at("b"));
        EXPECT_EQ(content.size(), 2);
        content.assign(content.size(), makeTable("c"));
        content.assign(content.size(), makeTable("c"));
        content.remove("c");
        EXPECT_ANY_THROW(content.at("c"));
    }

    TEST_F(LuaUiContentTest, IndexOf)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::Content::View content = makeContent(table);
        auto child = makeTable();
        content.assign(2, child);
        EXPECT_EQ(content.indexOf(child).value(), 2);
        EXPECT_TRUE(!content.indexOf(makeTable()).has_value());
    }

    TEST_F(LuaUiContentTest, BoundsChecks)
    {
        auto table = makeTable();
        LuaUi::Content::View content = makeContent(table);
        EXPECT_ANY_THROW(content.at(0));
        EXPECT_EQ(content.size(), 0);   
        content.assign(content.size(), makeTable());
        EXPECT_EQ(content.size(), 1);
        content.assign(content.size(), makeTable());
        EXPECT_EQ(content.size(), 2);
        content.assign(content.size(), makeTable());
        EXPECT_EQ(content.size(), 3);
        EXPECT_ANY_THROW(content.at(3));
        EXPECT_ANY_THROW(content.remove(3));
        content.remove(2);
        EXPECT_EQ(content.size(), 2);
        EXPECT_ANY_THROW(content.at(2));
    }
}
