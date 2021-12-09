#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/lua/luastate.hpp>

#include "testing_util.hpp"

namespace
{
    using namespace testing;

    TestFile counterFile(R"X(
x = 42
return {
    get = function() return x end,
    inc = function(v) x = x + v end
}
)X");

    TestFile invalidScriptFile("Invalid script");

    TestFile testsFile(R"X(
return {
    -- should work
    sin = function(x) return math.sin(x) end,
    requireMathSin = function(x) return require('math').sin(x) end,
    useCounter = function()
        local counter = require('aaa.counter')
        counter.inc(1)
        return counter.get()
    end,
    callRawset = function()
        t = {a = 1, b = 2}
        rawset(t, 'b', 3)
        return t.b
    end,
    print = print,

    -- should throw an error
    incorrectRequire = function() require('counter') end,
    modifySystemLib = function() math.sin = 5 end,
    rawsetSystemLib = function() rawset(math, 'sin', 5) end,
    callLoadstring = function() loadstring('print(1)') end,
    setSqr = function() require('sqrlib').sqr = math.sin end,
    setOmwName = function() require('openmw').name = 'abc' end,

    -- should work if API is registered
    sqr = function(x) return require('sqrlib').sqr(x) end,
    apiName = function() return require('test.api').name end
}
)X");

    struct LuaStateTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            {"aaa/counter.lua", &counterFile},
            {"bbb/tests.lua", &testsFile},
            {"invalid.lua", &invalidScriptFile}
        });

        LuaUtil::ScriptsConfiguration mCfg;
        LuaUtil::LuaState mLua{mVFS.get(), &mCfg};
    };

    TEST_F(LuaStateTest, Sandbox)
    {
        sol::table script1 = mLua.runInNewSandbox("aaa/counter.lua");

        EXPECT_EQ(LuaUtil::call(script1["get"]).get<int>(), 42);
        LuaUtil::call(script1["inc"], 3);
        EXPECT_EQ(LuaUtil::call(script1["get"]).get<int>(), 45);

        sol::table script2 = mLua.runInNewSandbox("aaa/counter.lua");
        EXPECT_EQ(LuaUtil::call(script2["get"]).get<int>(), 42);
        LuaUtil::call(script2["inc"], 1);
        EXPECT_EQ(LuaUtil::call(script2["get"]).get<int>(), 43);

        EXPECT_EQ(LuaUtil::call(script1["get"]).get<int>(), 45);
    }

    TEST_F(LuaStateTest, ToString)
    {
        EXPECT_EQ(LuaUtil::toString(sol::make_object(mLua.sol(), 3.14)), "3.14");
        EXPECT_EQ(LuaUtil::toString(sol::make_object(mLua.sol(), true)), "true");
        EXPECT_EQ(LuaUtil::toString(sol::nil), "nil");
        EXPECT_EQ(LuaUtil::toString(sol::make_object(mLua.sol(), "something")), "\"something\"");
    }

    TEST_F(LuaStateTest, ErrorHandling)
    {
        EXPECT_ERROR(mLua.runInNewSandbox("invalid.lua"), "[string \"invalid.lua\"]:1:");
    }

    TEST_F(LuaStateTest, CustomRequire)
    {
        sol::table script = mLua.runInNewSandbox("bbb/tests.lua");

        EXPECT_FLOAT_EQ(LuaUtil::call(script["sin"], 1).get<float>(),
                        -LuaUtil::call(script["requireMathSin"], -1).get<float>());

        EXPECT_EQ(LuaUtil::call(script["useCounter"]).get<int>(), 43);
        EXPECT_EQ(LuaUtil::call(script["useCounter"]).get<int>(), 44);
        {
            sol::table script2 = mLua.runInNewSandbox("bbb/tests.lua");
            EXPECT_EQ(LuaUtil::call(script2["useCounter"]).get<int>(), 43);
        }
        EXPECT_EQ(LuaUtil::call(script["useCounter"]).get<int>(), 45);

        EXPECT_ERROR(LuaUtil::call(script["incorrectRequire"]), "Resource 'counter.lua' not found");
    }

    TEST_F(LuaStateTest, ReadOnly)
    {
        sol::table script = mLua.runInNewSandbox("bbb/tests.lua");

        // rawset itself is allowed
        EXPECT_EQ(LuaUtil::call(script["callRawset"]).get<int>(), 3);

        // but read-only object can not be modified even with rawset
        EXPECT_ERROR(LuaUtil::call(script["rawsetSystemLib"]), "bad argument #1 to 'rawset' (table expected, got userdata)");
        EXPECT_ERROR(LuaUtil::call(script["modifySystemLib"]), "a userdata value");

        EXPECT_EQ(LuaUtil::getMutableFromReadOnly(LuaUtil::makeReadOnly(script)), script);
    }

    TEST_F(LuaStateTest, Print)
    {
        {
            sol::table script = mLua.runInNewSandbox("bbb/tests.lua");
            testing::internal::CaptureStdout();
            LuaUtil::call(script["print"], 1, 2, 3);
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_EQ(output, "[bbb/tests.lua]:\t1\t2\t3\n");
        }
        {
            sol::table script = mLua.runInNewSandbox("bbb/tests.lua", "prefix");
            testing::internal::CaptureStdout();
            LuaUtil::call(script["print"]);  // print with no arguments
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_EQ(output, "prefix[bbb/tests.lua]:\n");
        }
    }

    TEST_F(LuaStateTest, UnsafeFunction)
    {
        sol::table script = mLua.runInNewSandbox("bbb/tests.lua");
        EXPECT_ERROR(LuaUtil::call(script["callLoadstring"]), "a nil value");
    }

    TEST_F(LuaStateTest, ProvideAPI)
    {
        LuaUtil::LuaState lua(mVFS.get(), &mCfg);

        sol::table api1 = LuaUtil::makeReadOnly(lua.sol().create_table_with("name", "api1"));
        sol::table api2 = LuaUtil::makeReadOnly(lua.sol().create_table_with("name", "api2"));

        sol::table script1 = lua.runInNewSandbox("bbb/tests.lua", "", {{"test.api", api1}});

        lua.addCommonPackage(
            "sqrlib", lua.sol().create_table_with("sqr", [](int x) { return x * x; }));

        sol::table script2 = lua.runInNewSandbox("bbb/tests.lua", "", {{"test.api", api2}});

        EXPECT_ERROR(LuaUtil::call(script1["sqr"], 3), "Resource 'sqrlib.lua' not found");
        EXPECT_EQ(LuaUtil::call(script2["sqr"], 3).get<int>(), 9);

        EXPECT_EQ(LuaUtil::call(script1["apiName"]).get<std::string>(), "api1");
        EXPECT_EQ(LuaUtil::call(script2["apiName"]).get<std::string>(), "api2");
    }

    TEST_F(LuaStateTest, GetLuaVersion)
    {
        EXPECT_THAT(LuaUtil::getLuaVersion(), HasSubstr("Lua"));
    }

}
