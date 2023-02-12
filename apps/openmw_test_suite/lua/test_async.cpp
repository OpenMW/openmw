#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/lua/asyncpackage.hpp>
#include <components/lua/luastate.hpp>

#include "../testing_util.hpp"

namespace
{
    using namespace testing;
    using namespace TestingOpenMW;

    struct LuaCoroutineCallbackTest : Test
    {
        void SetUp() override
        {
            mLua.open_libraries(sol::lib::coroutine);
            mLua["callback"] = [&](sol::protected_function fn) -> LuaUtil::Callback {
                sol::table hiddenData(mLua, sol::create);
                hiddenData[LuaUtil::ScriptsContainer::sScriptIdKey] = sol::table(mLua, sol::create);
                return LuaUtil::Callback{ std::move(fn), hiddenData };
            };
            mLua["pass"] = [this](LuaUtil::Callback callback) { mCb = callback; };
        }

        sol::state mLua;
        LuaUtil::Callback mCb;
    };

    TEST_F(LuaCoroutineCallbackTest, CoroutineCallbacks)
    {
        internal::CaptureStdout();
        mLua.safe_script(R"X(
            local s = 'test'
            coroutine.wrap(function()
                pass(callback(function(v) print(s) end))
            end)()
        )X");
        mLua.collect_garbage();
        mCb.call();
        EXPECT_THAT(internal::GetCapturedStdout(), "test\n");
    }

    TEST_F(LuaCoroutineCallbackTest, ErrorInCoroutineCallbacks)
    {
        mLua.safe_script(R"X(
            coroutine.wrap(function()
                pass(callback(function() error('COROUTINE CALLBACK') end))
            end)()
        )X");
        mLua.collect_garbage();
        EXPECT_ERROR(mCb.call(), "COROUTINE CALLBACK");
    }
}
