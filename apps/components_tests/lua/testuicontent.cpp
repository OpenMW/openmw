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
        sol::protected_function mNew;
        LuaUiContentTest()
        {
            mLuaState.addInternalLibSearchPath(
                std::filesystem::path{ OPENMW_PROJECT_SOURCE_DIR } / "components" / "lua_ui");
            mNew = LuaUi::loadContentConstructor(&mLuaState);
        }

        LuaUi::ContentView makeContent(sol::table source)
        {
            auto result = mNew.call(source);
            if (result.get_type() != sol::type::table)
                throw std::logic_error("Expected table");
            return LuaUi::ContentView(result.get<sol::table>());
        }

        sol::table makeTable() { return sol::table(mLuaState.unsafeState(), sol::create); }
    };

    TEST_F(LuaUiContentTest, ProtectedMetatable)
    {
        sol::state_view sol = mLuaState.unsafeState();
        sol["makeContent"] = mNew;
        std::string testScript = R"(
            assert(not pcall(function() setmetatable(makeContent{}, {}) end), 'Metatable is not protected')
            assert(getmetatable(makeContent{}) == false, 'Metatable is not protected')
        )";
        EXPECT_NO_THROW(sol.safe_script(testScript));
    }

    TEST_F(LuaUiContentTest, Insert)
    {
        mLuaState.protectedCall([&](LuaUtil::LuaView& state) {
            sol::state_view& sol = state.sol();
            sol["makeContent"] = mNew;
            EXPECT_NO_THROW(sol.safe_script(R"(
                local content = makeContent({ { name = 'a' }, { name = 'b' }, { name = 'c' } })
                content:insert(2, { name = 'inserted' })
                assert(#content == 4, 'Not inserted')
                assert(content:indexOf('a') == 1, 'Wrong index for a')
                assert(content:indexOf('inserted') == 2, 'Wrong index for inserted')
                assert(content:indexOf('b') == 3, 'Wrong index for b')
                assert(content:indexOf('c') == 4, 'Wrong index for c')
                )"));
        });
    }

    TEST_F(LuaUiContentTest, MakeHole)
    {
        mLuaState.protectedCall([&](LuaUtil::LuaView& state) {
            sol::state_view& sol = state.sol();
            sol["makeContent"] = mNew;
            EXPECT_NO_THROW(sol.safe_script(R"(
                local content = makeContent({ {}, {} })
                local ok, value = pcall(function() return content[4] end)
                assert(ok, 'Out-of-bounds index raised an error')
                assert(value == nil, 'Out-of-bounds index did not return nil')
                )"));
        });
    }

    TEST_F(LuaUiContentTest, Create)
    {
        auto table = makeTable();
        table.add(makeTable());
        table.add(makeTable());
        table.add(makeTable());
        LuaUi::ContentView content = makeContent(table);
        EXPECT_EQ(content.size(), 3);
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
        mLuaState.protectedCall([&](LuaUtil::LuaView& state) {
            sol::state_view& sol = state.sol();
            sol["makeContent"] = mNew;
            EXPECT_NO_THROW(sol.safe_script(R"(
                local content = makeContent({ {}, { name = 'a' } })
                assert(content:indexOf('a') ~= nil, 'Could not find named table')
                content['a'] = nil
                assert(#content == 1, 'Failed to remove')
                content:add({ name = 'b' })
                content['b'] = {}
                assert(#content == 2, 'Failed to insert')
                content:add({ name = 'c' })
                content:add({ name = 'c' })
                content['c'] = nil
                assert(content:indexOf('c') == nil, 'Failed to remove value inserted twice'..#content)
                )"));
        });
    }

    TEST_F(LuaUiContentTest, IndexOf)
    {
        mLuaState.protectedCall([&](LuaUtil::LuaView& state) {
            sol::state_view& sol = state.sol();
            sol["makeContent"] = mNew;
            EXPECT_NO_THROW(sol.safe_script(R"(
                local content = makeContent({ {}, {}, {} })
                local child = {}
                content[3] = child
                assert(content:indexOf(child) == 3, 'Failed to assign')
                assert(content:indexOf({}) == nil, 'Found non-existent child')
                )"));
        });
    }

    TEST_F(LuaUiContentTest, BoundsChecks)
    {
        {
            auto table = makeTable();
            LuaUi::ContentView content = makeContent(table);
            EXPECT_ANY_THROW(content.at(0));
            EXPECT_EQ(content.size(), 0);
        }
        {
            auto table = makeTable();
            table[1] = makeTable();
            LuaUi::ContentView content = makeContent(table);
            EXPECT_EQ(content.size(), 1);
            EXPECT_ANY_THROW(content.at(1));
            content.at(0);
        }
    }
}
