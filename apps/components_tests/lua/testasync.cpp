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
            mLua.protectedCall([&](LuaUtil::LuaView& view) {
                sol::table hiddenData(view.sol(), sol::create);
                hiddenData[LuaUtil::ScriptsContainer::sScriptIdKey] = LuaUtil::ScriptId{};
                view.sol()["async"] = LuaUtil::getAsyncPackageInitializer(
                    view.sol(), []() { return 0.; }, []() { return 0.; })(hiddenData);
                view.sol()["pass"] = [&](const sol::table& t) { mCb = LuaUtil::Callback::fromLua(t); };
            });
        }

        LuaUtil::LuaState mLua{ nullptr, nullptr };
        LuaUtil::Callback mCb;
    };

    TEST_F(LuaCoroutineCallbackTest, CoroutineCallbacks)
    {
        internal::CaptureStdout();
        mLua.protectedCall([&](LuaUtil::LuaView& view) {
            view.sol().safe_script(R"X(
                local s = 'test'
                coroutine.wrap(function()
                    pass(async:callback(function(v) print(s) end))
                end)()
            )X");
            view.sol().collect_garbage();
            mCb.call();
        });
        EXPECT_THAT(internal::GetCapturedStdout(), "test\n");
    }

    TEST_F(LuaCoroutineCallbackTest, ErrorInCoroutineCallbacks)
    {
        mLua.protectedCall([&](LuaUtil::LuaView& view) {
            view.sol().safe_script(R"X(
                coroutine.wrap(function()
                    pass(async:callback(function() error('COROUTINE CALLBACK') end))
                end)()
            )X");
            view.sol().collect_garbage();
        });
        EXPECT_ERROR(mCb.call(), "COROUTINE CALLBACK");
    }
}
