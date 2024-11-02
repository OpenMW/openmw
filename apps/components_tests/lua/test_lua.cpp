#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/lua/luastate.hpp>
#include <components/testing/expecterror.hpp>
#include <components/testing/util.hpp>

namespace
{
    using namespace testing;

    constexpr VFS::Path::NormalizedView counterPath("aaa/counter.lua");

    TestingOpenMW::VFSTestFile counterFile(R"X(
x = 42
return {
    get = function() return x end,
    inc = function(v) x = x + v end
}
)X");

    constexpr VFS::Path::NormalizedView invalidPath("invalid.lua");

    TestingOpenMW::VFSTestFile invalidScriptFile("Invalid script");

    constexpr VFS::Path::NormalizedView testsPath("bbb/tests.lua");

    TestingOpenMW::VFSTestFile testsFile(R"X(
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
    modifySystemLib2 = function() math.__index.sin = 5 end,
    rawsetSystemLib = function() rawset(math, 'sin', 5) end,
    callLoadstring = function() loadstring('print(1)') end,
    setSqr = function() require('sqrlib').sqr = math.sin end,
    setOmwName = function() require('openmw').name = 'abc' end,

    -- should work if API is registered
    sqr = function(x) return require('sqrlib').sqr(x) end,
    apiName = function() return require('test.api').name end
}
)X");

    constexpr VFS::Path::NormalizedView metaIndexErrorPath("metaindexerror.lua");

    TestingOpenMW::VFSTestFile metaIndexErrorFile(
        "return setmetatable({}, { __index = function(t, key) error('meta index error') end })");

    std::string genBigScript()
    {
        std::stringstream buf;
        buf << "return function()\n";
        buf << "  x = {}\n";
        for (int i = 0; i < 1000; ++i)
            buf << "  x[" << i * 2 << "] = " << i << "\n";
        buf << "  return x\n";
        buf << "end\n";
        return buf.str();
    }

    constexpr VFS::Path::NormalizedView bigPath("big.lua");

    TestingOpenMW::VFSTestFile bigScriptFile(genBigScript());

    constexpr VFS::Path::NormalizedView requireBigPath("requirebig.lua");

    TestingOpenMW::VFSTestFile requireBigScriptFile("local x = require('big') ; return {x}");

    struct LuaStateTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = TestingOpenMW::createTestVFS({
            { counterPath, &counterFile },
            { testsPath, &testsFile },
            { invalidPath, &invalidScriptFile },
            { bigPath, &bigScriptFile },
            { requireBigPath, &requireBigScriptFile },
            { metaIndexErrorPath, &metaIndexErrorFile },
        });

        LuaUtil::ScriptsConfiguration mCfg;
        LuaUtil::LuaState mLua{ mVFS.get(), &mCfg };
    };

    TEST_F(LuaStateTest, Sandbox)
    {
        const VFS::Path::Normalized path(counterPath);
        sol::table script1 = mLua.runInNewSandbox(path);

        EXPECT_EQ(LuaUtil::call(script1["get"]).get<int>(), 42);
        LuaUtil::call(script1["inc"], 3);
        EXPECT_EQ(LuaUtil::call(script1["get"]).get<int>(), 45);

        sol::table script2 = mLua.runInNewSandbox(path);
        EXPECT_EQ(LuaUtil::call(script2["get"]).get<int>(), 42);
        LuaUtil::call(script2["inc"], 1);
        EXPECT_EQ(LuaUtil::call(script2["get"]).get<int>(), 43);

        EXPECT_EQ(LuaUtil::call(script1["get"]).get<int>(), 45);
    }

    TEST_F(LuaStateTest, ToString)
    {
        EXPECT_EQ(LuaUtil::toString(sol::make_object(mLua.unsafeState(), 3.14)), "3.14");
        EXPECT_EQ(LuaUtil::toString(sol::make_object(mLua.unsafeState(), true)), "true");
        EXPECT_EQ(LuaUtil::toString(sol::nil), "nil");
        EXPECT_EQ(LuaUtil::toString(sol::make_object(mLua.unsafeState(), "something")), "\"something\"");
    }

    TEST_F(LuaStateTest, Cast)
    {
        EXPECT_EQ(LuaUtil::cast<int>(sol::make_object(mLua.unsafeState(), 3.14)), 3);
        EXPECT_ERROR(LuaUtil::cast<int>(sol::make_object(mLua.unsafeState(), "3.14")),
            "Value \"\"3.14\"\" can not be casted to int");
        EXPECT_ERROR(LuaUtil::cast<std::string_view>(sol::make_object(mLua.unsafeState(), sol::nil)),
            "Value \"nil\" can not be casted to string");
        EXPECT_ERROR(LuaUtil::cast<std::string>(sol::make_object(mLua.unsafeState(), sol::nil)),
            "Value \"nil\" can not be casted to string");
        EXPECT_ERROR(LuaUtil::cast<sol::table>(sol::make_object(mLua.unsafeState(), sol::nil)),
            "Value \"nil\" can not be casted to sol::table");
        EXPECT_ERROR(LuaUtil::cast<sol::function>(sol::make_object(mLua.unsafeState(), "3.14")),
            "Value \"\"3.14\"\" can not be casted to sol::function");
        EXPECT_ERROR(LuaUtil::cast<sol::protected_function>(sol::make_object(mLua.unsafeState(), "3.14")),
            "Value \"\"3.14\"\" can not be casted to sol::function");
    }

    TEST_F(LuaStateTest, ErrorHandling)
    {
        const VFS::Path::Normalized path("invalid.lua");
        EXPECT_ERROR(mLua.runInNewSandbox(path), "[string \"invalid.lua\"]:1:");
    }

    TEST_F(LuaStateTest, CustomRequire)
    {
        const VFS::Path::Normalized path("bbb/tests.lua");
        sol::table script = mLua.runInNewSandbox(path);

        EXPECT_FLOAT_EQ(
            LuaUtil::call(script["sin"], 1).get<float>(), -LuaUtil::call(script["requireMathSin"], -1).get<float>());

        EXPECT_EQ(LuaUtil::call(script["useCounter"]).get<int>(), 43);
        EXPECT_EQ(LuaUtil::call(script["useCounter"]).get<int>(), 44);
        {
            sol::table script2 = mLua.runInNewSandbox(path);
            EXPECT_EQ(LuaUtil::call(script2["useCounter"]).get<int>(), 43);
        }
        EXPECT_EQ(LuaUtil::call(script["useCounter"]).get<int>(), 45);

        EXPECT_ERROR(LuaUtil::call(script["incorrectRequire"]), "module not found: counter");
    }

    TEST_F(LuaStateTest, ReadOnly)
    {
        const VFS::Path::Normalized path("bbb/tests.lua");
        sol::table script = mLua.runInNewSandbox(path);

        // rawset itself is allowed
        EXPECT_EQ(LuaUtil::call(script["callRawset"]).get<int>(), 3);

        // but read-only object can not be modified even with rawset
        EXPECT_ERROR(
            LuaUtil::call(script["rawsetSystemLib"]), "bad argument #1 to 'rawset' (table expected, got userdata)");
        EXPECT_ERROR(LuaUtil::call(script["modifySystemLib"]), "a userdata value");
        EXPECT_ERROR(LuaUtil::call(script["modifySystemLib2"]), "a nil value");

        EXPECT_EQ(LuaUtil::getMutableFromReadOnly(LuaUtil::makeReadOnly(script)), script);
    }

    TEST_F(LuaStateTest, Print)
    {
        const VFS::Path::Normalized path("bbb/tests.lua");
        {
            sol::table script = mLua.runInNewSandbox(path);
            testing::internal::CaptureStdout();
            LuaUtil::call(script["print"], 1, 2, 3);
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_EQ(output, "unnamed:\t1\t2\t3\n");
        }
        {
            sol::table script = mLua.runInNewSandbox(path, "prefix");
            testing::internal::CaptureStdout();
            LuaUtil::call(script["print"]); // print with no arguments
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_EQ(output, "prefix:\n");
        }
    }

    TEST_F(LuaStateTest, UnsafeFunction)
    {
        const VFS::Path::Normalized path("bbb/tests.lua");
        sol::table script = mLua.runInNewSandbox(path);
        EXPECT_ERROR(LuaUtil::call(script["callLoadstring"]), "a nil value");
    }

    TEST_F(LuaStateTest, ProvideAPI)
    {
        LuaUtil::LuaState lua(mVFS.get(), &mCfg);
        lua.protectedCall([&](LuaUtil::LuaView& view) {
            sol::table api1 = LuaUtil::makeReadOnly(view.sol().create_table_with("name", "api1"));
            sol::table api2 = LuaUtil::makeReadOnly(view.sol().create_table_with("name", "api2"));

            const VFS::Path::Normalized path("bbb/tests.lua");

            sol::table script1 = lua.runInNewSandbox(path, "", { { "test.api", api1 } });

            lua.addCommonPackage("sqrlib", view.sol().create_table_with("sqr", [](int x) { return x * x; }));

            sol::table script2 = lua.runInNewSandbox(path, "", { { "test.api", api2 } });

            EXPECT_ERROR(LuaUtil::call(script1["sqr"], 3), "module not found: sqrlib");
            EXPECT_EQ(LuaUtil::call(script2["sqr"], 3).get<int>(), 9);

            EXPECT_EQ(LuaUtil::call(script1["apiName"]).get<std::string>(), "api1");
            EXPECT_EQ(LuaUtil::call(script2["apiName"]).get<std::string>(), "api2");
        });
    }

    TEST_F(LuaStateTest, GetLuaVersion)
    {
        EXPECT_THAT(LuaUtil::getLuaVersion(), HasSubstr("Lua"));
    }

    TEST_F(LuaStateTest, RemovedScriptsGarbageCollecting)
    {
        auto getMem = [&] {
            for (int i = 0; i < 5; ++i)
                lua_gc(mLua.unsafeState(), LUA_GCCOLLECT, 0);
            return mLua.getTotalMemoryUsage();
        };
        int64_t memWithScript;
        const VFS::Path::Normalized path("requireBig.lua");
        {
            sol::object s = mLua.runInNewSandbox(path);
            memWithScript = getMem();
        }
        for (int i = 0; i < 100; ++i) // run many times to make small memory leaks visible
            mLua.runInNewSandbox(path);
        int64_t memWithoutScript = getMem();
        // At this moment all instances of the script should be garbage-collected.
        EXPECT_LT(memWithoutScript, memWithScript);
    }

    TEST_F(LuaStateTest, SafeIndexMetamethod)
    {
        const VFS::Path::Normalized path("metaIndexError.lua");
        sol::table t = mLua.runInNewSandbox(path);
        // without safe get we crash here
        EXPECT_ERROR(LuaUtil::safeGet(t, "any key"), "meta index error");
    }
}
