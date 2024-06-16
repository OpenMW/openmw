#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/lua/asyncpackage.hpp>
#include <components/lua/luastate.hpp>
#include <components/testing/expecterror.hpp>

namespace
{
    using namespace testing;

    struct LuaCoroutineCallbackTest : Test
    {
        void SetUp() override
        {
            mLua.sol()["callback"] = [&](sol::protected_function fn) -> LuaUtil::Callback {
                sol::table hiddenData(mLua.sol(), sol::create);
                hiddenData[LuaUtil::ScriptsContainer::sScriptIdKey] = LuaUtil::ScriptId{};
                return LuaUtil::Callback{ std::move(fn), hiddenData };
            };
            mLua.sol()["pass"] = [this](LuaUtil::Callback callback) { mCb = callback; };
        }

        LuaUtil::LuaState mLua{ nullptr, nullptr };
        LuaUtil::Callback mCb;
    };

    TEST_F(LuaCoroutineCallbackTest, CoroutineCallbacks)
    {
        internal::CaptureStdout();
        mLua.sol().safe_script(R"X(
            local s = 'test'
            coroutine.wrap(function()
                pass(callback(function(v) print(s) end))
            end)()
        )X");
        mLua.sol().collect_garbage();
        mCb.call();
        EXPECT_THAT(internal::GetCapturedStdout(), "test\n");
    }

    TEST_F(LuaCoroutineCallbackTest, ErrorInCoroutineCallbacks)
    {
        mLua.sol().safe_script(R"X(
            coroutine.wrap(function()
                pass(callback(function() error('COROUTINE CALLBACK') end))
            end)()
        )X");
        mLua.sol().collect_garbage();
        EXPECT_ERROR(mCb.call(), "COROUTINE CALLBACK");
    }
}
