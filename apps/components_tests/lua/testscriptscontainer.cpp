#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/esm/luascripts.hpp>

#include <components/lua/asyncpackage.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>
#include <components/lua/scripttracker.hpp>

#include <components/testing/util.hpp>

namespace
{
    using namespace testing;
    using namespace TestingOpenMW;

    constexpr VFS::Path::NormalizedView invalidPath("invalid.lua");

    VFSTestFile invalidScript("not a script");

    constexpr VFS::Path::NormalizedView incorrectPath("incorrect.lua");

    VFSTestFile incorrectScript(
        "return { incorrectSection = {}, engineHandlers = { incorrectHandler = function() end } }");

    constexpr VFS::Path::NormalizedView emptyPath("empty.lua");

    VFSTestFile emptyScript("");

    constexpr VFS::Path::NormalizedView test1Path("test1.lua");
    constexpr VFS::Path::NormalizedView test2Path("test2.lua");

    VFSTestFile testScript(R"X(
return {
    engineHandlers = {
        onUpdate = function(dt) print(' update ' .. tostring(dt)) end,
        onLoad = function() print('load') end,
    },
    eventHandlers = {
        Event1 = function(eventData) print(' event1 ' .. tostring(eventData.x)) end,
        Event2 = function(eventData) print(' event2 ' .. tostring(eventData.x)) end,
        Print = function() print('print') end
    }
}
)X");

    constexpr VFS::Path::NormalizedView stopEventPath("stopevent.lua");

    VFSTestFile stopEventScript(R"X(
return {
    eventHandlers = {
        Event1 = function(eventData)
            print(' event1 ' .. tostring(eventData.x))
            return eventData.x >= 1
        end
    }
}
)X");

    constexpr VFS::Path::NormalizedView loadSave1Path("loadsave1.lua");
    constexpr VFS::Path::NormalizedView loadSave2Path("loadsave2.lua");

    VFSTestFile loadSaveScript(R"X(
x = 0
y = 0
return {
    engineHandlers = {
        onSave = function(state)
            return {x = x, y = y}
        end,
        onLoad = function(state)
            x, y = state.x, state.y
        end
    },
    eventHandlers = {
        Set = function(eventData)
            eventData.n = eventData.n - 1
            if eventData.n == 0 then
                x, y = eventData.x, eventData.y
            end
        end,
        Print = function()
            print(x, y)
        end
    }
}
)X");

    constexpr VFS::Path::NormalizedView testInterfacePath("testinterface.lua");

    VFSTestFile interfaceScript(R"X(
return {
    interfaceName = "TestInterface",
    interface = {
        fn = function(x) print('FN', x) end,
        value = 3.5
    },
}
)X");

    constexpr VFS::Path::NormalizedView overrideInterfacePath("overrideinterface.lua");

    VFSTestFile overrideInterfaceScript(R"X(
local old = nil
local interface = {
    fn = function(x)
        print('NEW FN', x)
        old.fn(x)
    end,
    value,
}
return {
    interfaceName = "TestInterface",
    interface = interface,
    engineHandlers = {
        onInit = function() print('init') end,
        onLoad = function() print('load') end,
        onInterfaceOverride = function(oldInterface)
            print('override')
            old = oldInterface
            interface.value = oldInterface.value + 1
        end
    },
}
)X");

    constexpr VFS::Path::NormalizedView useInterfacePath("useinterface.lua");

    VFSTestFile useInterfaceScript(R"X(
local interfaces = require('openmw.interfaces')
return {
    engineHandlers = {
        onUpdate = function()
            interfaces.TestInterface.fn(interfaces.TestInterface.value)
        end,
    },
}
)X");

    constexpr VFS::Path::NormalizedView unloadPath("unload.lua");

    VFSTestFile unloadScript(R"X(
x = 0
y = 0
z = 0
return {
    engineHandlers = {
        onSave = function(state)
            print('saving', x, y, z)
            return {x = x, y = y}
        end,
        onLoad = function(state)
            x, y = state.x, state.y
            print('loaded', x, y, z)
        end
    },
    eventHandlers = {
        Set = function(eventData)
            x, y, z = eventData.x, eventData.y, eventData.z
        end
    }
}
)X");

    constexpr VFS::Path::NormalizedView customDataPath("customdata.lua");

    VFSTestFile customDataScript(R"X(
data = nil
return {
    engineHandlers = {
        onSave = function()
            return data
        end,
        onLoad = function(state)
            data = state
        end,
        onInit = function(state)
            data = state
        end
    },
    eventHandlers = {
        WakeUp = function()
        end
    }
}
)X");

    struct LuaScriptsContainerTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            { invalidPath, &invalidScript },
            { incorrectPath, &incorrectScript },
            { emptyPath, &emptyScript },
            { test1Path, &testScript },
            { test2Path, &testScript },
            { stopEventPath, &stopEventScript },
            { loadSave1Path, &loadSaveScript },
            { loadSave2Path, &loadSaveScript },
            { testInterfacePath, &interfaceScript },
            { overrideInterfacePath, &overrideInterfaceScript },
            { useInterfacePath, &useInterfaceScript },
            { unloadPath, &unloadScript },
            { customDataPath, &customDataScript },
        });

        LuaUtil::ScriptsConfiguration mCfg;
        LuaUtil::LuaState mLua{ mVFS.get(), &mCfg };

        LuaScriptsContainerTest()
        {
            ESM::LuaScriptsCfg cfg;
            LuaUtil::parseOMWScripts(cfg, R"X(
CUSTOM: invalid.lua
CUSTOM: incorrect.lua
CUSTOM: empty.lua
CUSTOM: test1.lua
CUSTOM: stopEvent.lua
CUSTOM: test2.lua
NPC: loadSave1.lua
CUSTOM, NPC: loadSave2.lua
CUSTOM, PLAYER: testInterface.lua
CUSTOM, PLAYER: overrideInterface.lua
CUSTOM, PLAYER: useInterface.lua
CUSTOM: unload.lua
CUSTOM: customdata.lua
)X");
            mCfg.init(std::move(cfg), false);
        }

        int getId(VFS::Path::NormalizedView path) const
        {
            const std::optional<int> id = mCfg.findId(path);
            if (!id.has_value())
                throw std::invalid_argument("Script id is not found: " + std::string(path.value()));
            return *id;
        }
    };

    TEST_F(LuaScriptsContainerTest, addCustomScriptShouldNotStartInvalidScript)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        testing::internal::CaptureStdout();
        EXPECT_FALSE(scripts.addCustomScript(getId(invalidPath)));
        std::string output = testing::internal::GetCapturedStdout();
        EXPECT_THAT(output, HasSubstr("Can't start Test[invalid.lua]"));
    }

    TEST_F(LuaScriptsContainerTest, addCustomScriptShouldNotSuportScriptsWithInvalidHandlerAndSection)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        testing::internal::CaptureStdout();
        EXPECT_TRUE(scripts.addCustomScript(getId(incorrectPath)));
        std::string output = testing::internal::GetCapturedStdout();
        EXPECT_THAT(output, HasSubstr("Not supported handler 'incorrectHandler' in Test[incorrect.lua]"));
        EXPECT_THAT(output, HasSubstr("Not supported section 'incorrectSection' in Test[incorrect.lua]"));
    }

    TEST_F(LuaScriptsContainerTest, addCustomScriptShouldReturnFalseForDuplicates)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        EXPECT_TRUE(scripts.addCustomScript(getId(emptyPath)));
        EXPECT_FALSE(scripts.addCustomScript(getId(emptyPath)));
    }

    TEST_F(LuaScriptsContainerTest, CallHandler)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        testing::internal::CaptureStdout();
        EXPECT_TRUE(scripts.addCustomScript(getId(test1Path)));
        EXPECT_TRUE(scripts.addCustomScript(getId(stopEventPath)));
        EXPECT_TRUE(scripts.addCustomScript(getId(test2Path)));
        scripts.update(1.5f);
        EXPECT_EQ(internal::GetCapturedStdout(),
            "Test[test1.lua]:\t update 1.5\n"
            "Test[test2.lua]:\t update 1.5\n");
    }

    TEST_F(LuaScriptsContainerTest, CallEvent)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");

        EXPECT_TRUE(scripts.addCustomScript(getId(test1Path)));
        EXPECT_TRUE(scripts.addCustomScript(getId(stopEventPath)));
        EXPECT_TRUE(scripts.addCustomScript(getId(test2Path)));

        sol::state_view sol = mLua.unsafeState();
        std::string x0 = LuaUtil::serialize(sol.create_table_with("x", 0.5));
        std::string x1 = LuaUtil::serialize(sol.create_table_with("x", 1.5));

        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("SomeEvent", x1);
            EXPECT_EQ(internal::GetCapturedStdout(), "");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event1", x1);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test2.lua]:\t event1 1.5\n"
                "Test[stopevent.lua]:\t event1 1.5\n"
                "Test[test1.lua]:\t event1 1.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event2", x1);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test2.lua]:\t event2 1.5\n"
                "Test[test1.lua]:\t event2 1.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event1", x0);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test2.lua]:\t event1 0.5\n"
                "Test[stopevent.lua]:\t event1 0.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event2", x0);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test2.lua]:\t event2 0.5\n"
                "Test[test1.lua]:\t event2 0.5\n");
        }
    }

    TEST_F(LuaScriptsContainerTest, RemoveScript)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");

        EXPECT_TRUE(scripts.addCustomScript(getId(test1Path)));
        EXPECT_TRUE(scripts.addCustomScript(getId(stopEventPath)));
        EXPECT_TRUE(scripts.addCustomScript(getId(test2Path)));

        sol::state_view sol = mLua.unsafeState();
        std::string x = LuaUtil::serialize(sol.create_table_with("x", 0.5));

        {
            testing::internal::CaptureStdout();
            scripts.update(1.5f);
            scripts.receiveEvent("Event1", x);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test1.lua]:\t update 1.5\n"
                "Test[test2.lua]:\t update 1.5\n"
                "Test[test2.lua]:\t event1 0.5\n"
                "Test[stopevent.lua]:\t event1 0.5\n");
        }
        {
            testing::internal::CaptureStdout();
            const int stopEventScriptId = getId(stopEventPath);
            EXPECT_TRUE(scripts.hasScript(stopEventScriptId));
            scripts.removeScript(stopEventScriptId);
            EXPECT_FALSE(scripts.hasScript(stopEventScriptId));
            scripts.update(1.5f);
            scripts.receiveEvent("Event1", x);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test1.lua]:\t update 1.5\n"
                "Test[test2.lua]:\t update 1.5\n"
                "Test[test2.lua]:\t event1 0.5\n"
                "Test[test1.lua]:\t event1 0.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.removeScript(getId(test1Path));
            scripts.update(1.5f);
            scripts.receiveEvent("Event1", x);
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test2.lua]:\t update 1.5\n"
                "Test[test2.lua]:\t event1 0.5\n");
        }
    }

    TEST_F(LuaScriptsContainerTest, AutoStart)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        scripts.setAutoStartConf(mCfg.getPlayerConf());
        testing::internal::CaptureStdout();
        scripts.addAutoStartedScripts();
        scripts.update(1.5f);
        EXPECT_EQ(internal::GetCapturedStdout(),
            "Test[overrideinterface.lua]:\toverride\n"
            "Test[overrideinterface.lua]:\tinit\n"
            "Test[overrideinterface.lua]:\tNEW FN\t4.5\n"
            "Test[testinterface.lua]:\tFN\t4.5\n");
    }

    TEST_F(LuaScriptsContainerTest, Interface)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        scripts.setAutoStartConf(mCfg.getLocalConf(ESM::REC_CREA, ESM::RefId(), ESM::RefNum()));
        const int addIfaceId = getId(testInterfacePath);
        const int overrideIfaceId = getId(overrideInterfacePath);
        const int useIfaceId = getId(useInterfacePath);

        testing::internal::CaptureStdout();
        scripts.addAutoStartedScripts();
        scripts.update(1.5f);
        EXPECT_EQ(internal::GetCapturedStdout(), "");

        testing::internal::CaptureStdout();
        EXPECT_TRUE(scripts.addCustomScript(addIfaceId));
        EXPECT_TRUE(scripts.addCustomScript(overrideIfaceId));
        EXPECT_TRUE(scripts.addCustomScript(useIfaceId));
        scripts.update(1.5f);
        scripts.removeScript(overrideIfaceId);
        scripts.update(1.5f);
        EXPECT_EQ(internal::GetCapturedStdout(),
            "Test[overrideinterface.lua]:\toverride\n"
            "Test[overrideinterface.lua]:\tinit\n"
            "Test[overrideinterface.lua]:\tNEW FN\t4.5\n"
            "Test[testinterface.lua]:\tFN\t4.5\n"
            "Test[testinterface.lua]:\tFN\t3.5\n");
    }

    TEST_F(LuaScriptsContainerTest, LoadSave)
    {
        LuaUtil::ScriptsContainer scripts1(&mLua, "Test");
        LuaUtil::ScriptsContainer scripts2(&mLua, "Test");
        LuaUtil::ScriptsContainer scripts3(&mLua, "Test");
        scripts1.setAutoStartConf(mCfg.getLocalConf(ESM::REC_NPC_, ESM::RefId(), ESM::RefNum()));
        scripts2.setAutoStartConf(mCfg.getLocalConf(ESM::REC_NPC_, ESM::RefId(), ESM::RefNum()));
        scripts3.setAutoStartConf(mCfg.getPlayerConf());

        scripts1.addAutoStartedScripts();
        EXPECT_TRUE(scripts1.addCustomScript(getId(test1Path)));

        sol::state_view sol = mLua.unsafeState();
        scripts1.receiveEvent("Set", LuaUtil::serialize(sol.create_table_with("n", 1, "x", 0.5, "y", 3.5)));
        scripts1.receiveEvent("Set", LuaUtil::serialize(sol.create_table_with("n", 2, "x", 2.5, "y", 1.5)));

        ESM::LuaScripts data;
        scripts1.save(data);

        {
            testing::internal::CaptureStdout();
            scripts2.load(data);
            scripts2.receiveEvent("Print", "");
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[test1.lua]:\tload\n"
                "Test[loadsave2.lua]:\t0.5\t3.5\n"
                "Test[loadsave1.lua]:\t2.5\t1.5\n"
                "Test[test1.lua]:\tprint\n");
            EXPECT_FALSE(scripts2.hasScript(getId(testInterfacePath)));
        }
        {
            testing::internal::CaptureStdout();
            scripts3.load(data);
            scripts3.receiveEvent("Print", "");
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Ignoring Test[loadsave1.lua]; this script is not allowed here\n"
                "Test[test1.lua]:\tload\n"
                "Test[overrideinterface.lua]:\toverride\n"
                "Test[overrideinterface.lua]:\tinit\n"
                "Test[loadsave2.lua]:\t0.5\t3.5\n"
                "Test[test1.lua]:\tprint\n");
            EXPECT_TRUE(scripts3.hasScript(getId(testInterfacePath)));
        }
    }

    TEST_F(LuaScriptsContainerTest, Timers)
    {
        using TimerType = LuaUtil::ScriptsContainer::TimerType;
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        const int test1Id = getId(test1Path);
        const int test2Id = getId(test2Path);

        testing::internal::CaptureStdout();
        EXPECT_TRUE(scripts.addCustomScript(test1Id));
        EXPECT_TRUE(scripts.addCustomScript(test2Id));
        EXPECT_EQ(internal::GetCapturedStdout(), "");

        int counter1 = 0, counter2 = 0, counter3 = 0, counter4 = 0;
        sol::function fn1 = sol::make_object(mLua.unsafeState(), [&]() { counter1++; });
        sol::function fn2 = sol::make_object(mLua.unsafeState(), [&]() { counter2++; });
        sol::function fn3 = sol::make_object(mLua.unsafeState(), [&](int d) { counter3 += d; });
        sol::function fn4 = sol::make_object(mLua.unsafeState(), [&](int d) { counter4 += d; });

        scripts.registerTimerCallback(test1Id, "A", fn3);
        scripts.registerTimerCallback(test1Id, "B", fn4);
        scripts.registerTimerCallback(test2Id, "B", fn3);
        scripts.registerTimerCallback(test2Id, "A", fn4);

        scripts.processTimers(1, 2);

        scripts.setupSerializableTimer(
            TimerType::SIMULATION_TIME, 10, test1Id, "B", sol::make_object(mLua.unsafeState(), 3));
        scripts.setupSerializableTimer(TimerType::GAME_TIME, 10, test2Id, "B", sol::make_object(mLua.unsafeState(), 4));
        scripts.setupSerializableTimer(
            TimerType::SIMULATION_TIME, 5, test1Id, "A", sol::make_object(mLua.unsafeState(), 1));
        scripts.setupSerializableTimer(TimerType::GAME_TIME, 5, test2Id, "A", sol::make_object(mLua.unsafeState(), 2));
        scripts.setupSerializableTimer(
            TimerType::SIMULATION_TIME, 15, test1Id, "A", sol::make_object(mLua.unsafeState(), 10));
        scripts.setupSerializableTimer(
            TimerType::SIMULATION_TIME, 15, test1Id, "B", sol::make_object(mLua.unsafeState(), 20));

        scripts.setupUnsavableTimer(TimerType::SIMULATION_TIME, 10, test2Id, fn2);
        scripts.setupUnsavableTimer(TimerType::GAME_TIME, 10, test1Id, fn2);
        scripts.setupUnsavableTimer(TimerType::SIMULATION_TIME, 5, test2Id, fn1);
        scripts.setupUnsavableTimer(TimerType::GAME_TIME, 5, test1Id, fn1);
        scripts.setupUnsavableTimer(TimerType::SIMULATION_TIME, 15, test2Id, fn1);

        EXPECT_EQ(counter1, 0);
        EXPECT_EQ(counter3, 0);

        scripts.processTimers(6, 4);

        EXPECT_EQ(counter1, 1);
        EXPECT_EQ(counter3, 1);
        EXPECT_EQ(counter4, 0);

        scripts.processTimers(6, 8);

        EXPECT_EQ(counter1, 2);
        EXPECT_EQ(counter2, 0);
        EXPECT_EQ(counter3, 1);
        EXPECT_EQ(counter4, 2);

        scripts.processTimers(11, 12);

        EXPECT_EQ(counter1, 2);
        EXPECT_EQ(counter2, 2);
        EXPECT_EQ(counter3, 5);
        EXPECT_EQ(counter4, 5);

        testing::internal::CaptureStdout();
        ESM::LuaScripts data;
        scripts.save(data);
        scripts.load(data);
        scripts.registerTimerCallback(test1Id, "B", fn4);
        EXPECT_EQ(internal::GetCapturedStdout(), "Test[test1.lua]:\tload\nTest[test2.lua]:\tload\n");

        testing::internal::CaptureStdout();
        scripts.processTimers(20, 20);
        EXPECT_EQ(internal::GetCapturedStdout(), "Test[test1.lua] callTimer failed: Callback 'A' doesn't exist\n");

        EXPECT_EQ(counter1, 2);
        EXPECT_EQ(counter2, 2);
        EXPECT_EQ(counter3, 5);
        EXPECT_EQ(counter4, 25);
    }

    TEST_F(LuaScriptsContainerTest, CallbackWrapper)
    {
        sol::state_view view = mLua.unsafeState();
        LuaUtil::Callback callback{ view["print"], sol::table(view, sol::create) };
        callback.mHiddenData[LuaUtil::ScriptsContainer::sScriptDebugNameKey] = "some_script.lua";
        callback.mHiddenData[LuaUtil::ScriptsContainer::sScriptIdKey] = LuaUtil::ScriptId{ nullptr, 0 };

        testing::internal::CaptureStdout();
        callback.call(1.5);
        EXPECT_EQ(internal::GetCapturedStdout(), "1.5\n");

        testing::internal::CaptureStdout();
        callback.call(1.5, 2.5);
        EXPECT_EQ(internal::GetCapturedStdout(), "1.5\t2.5\n");

        const Debug::Level level = std::exchange(Log::sMinDebugLevel, Debug::All);

        testing::internal::CaptureStdout();
        callback.mHiddenData[LuaUtil::ScriptsContainer::sScriptIdKey] = sol::nil;
        callback.call(1.5, 2.5);
        EXPECT_EQ(internal::GetCapturedStdout(), "Ignored callback to the removed script some_script.lua\n");

        Log::sMinDebugLevel = level;
    }

    TEST_F(LuaScriptsContainerTest, Unload)
    {
        LuaUtil::ScriptTracker tracker;
        LuaUtil::ScriptsContainer scripts1(&mLua, "Test", &tracker, false);

        EXPECT_TRUE(scripts1.addCustomScript(*mCfg.findId(unloadPath)));
        EXPECT_EQ(tracker.size(), 1);

        mLua.protectedCall([&](LuaUtil::LuaView& lua) {
            scripts1.receiveEvent("Set", LuaUtil::serialize(lua.sol().create_table_with("x", 3, "y", 2, "z", 1)));
            testing::internal::CaptureStdout();
            for (int i = 0; i < 600; ++i)
                tracker.unloadInactiveScripts(lua);
            EXPECT_EQ(tracker.size(), 0);
            scripts1.receiveEvent("Set", LuaUtil::serialize(lua.sol().create_table_with("x", 10, "y", 20, "z", 30)));
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[unload.lua]:\tsaving\t3\t2\t1\n"
                "Test[unload.lua]:\tloaded\t3\t2\t0\n");
        });
        EXPECT_EQ(tracker.size(), 1);
        ESM::LuaScripts data;
        scripts1.save(data);
        EXPECT_EQ(tracker.size(), 1);
        mLua.protectedCall([&](LuaUtil::LuaView& lua) {
            for (int i = 0; i < 600; ++i)
                tracker.unloadInactiveScripts(lua);
        });
        EXPECT_EQ(tracker.size(), 0);
        scripts1.load(data);
        EXPECT_EQ(tracker.size(), 0);
    }

    TEST_F(LuaScriptsContainerTest, LoadOrderChange)
    {
        LuaUtil::ScriptTracker tracker;
        LuaUtil::ScriptsContainer scripts1(&mLua, "Test", &tracker, false);
        LuaUtil::BasicSerializer serializer1;
        LuaUtil::BasicSerializer serializer2([](int contentFileIndex) -> int {
            if (contentFileIndex == 12)
                return 34;
            else if (contentFileIndex == 37)
                return 12;
            return contentFileIndex;
        });
        scripts1.setSerializer(&serializer1);
        scripts1.setSavedDataDeserializer(&serializer2);

        mLua.protectedCall([&](LuaUtil::LuaView& lua) {
            sol::object id1 = sol::make_object_userdata(lua.sol(), ESM::RefNum{ 42, 12 });
            sol::object id2 = sol::make_object_userdata(lua.sol(), ESM::RefNum{ 13, 37 });
            sol::table table = lua.newTable();
            table[id1] = id2;
            LuaUtil::BinaryData serialized = LuaUtil::serialize(table, &serializer1);

            EXPECT_TRUE(scripts1.addCustomScript(*mCfg.findId(customDataPath), serialized));
            EXPECT_EQ(tracker.size(), 1);
            for (int i = 0; i < 600; ++i)
                tracker.unloadInactiveScripts(lua);
            EXPECT_EQ(tracker.size(), 0);
            scripts1.receiveEvent("WakeUp", {});
            EXPECT_EQ(tracker.size(), 1);
        });

        ESM::LuaScripts data1;
        ESM::LuaScripts data2;
        scripts1.save(data1);
        scripts1.load(data1);
        scripts1.save(data2);
        EXPECT_NE(data1.mScripts[0].mData, data2.mScripts[0].mData);

        mLua.protectedCall([&](LuaUtil::LuaView& lua) {
            sol::object deserialized = LuaUtil::deserialize(lua.sol(), data2.mScripts[0].mData, &serializer1);
            EXPECT_TRUE(deserialized.is<sol::table>());
            sol::table table = deserialized;
            if (!table.empty())
            {
                const auto [key, value] = *table.cbegin();
                EXPECT_TRUE(key.is<ESM::RefNum>());
                EXPECT_TRUE(value.is<ESM::RefNum>());
                EXPECT_EQ(key.as<ESM::RefNum>(), (ESM::RefNum{ 42, 34 }));
                EXPECT_EQ(value.as<ESM::RefNum>(), (ESM::RefNum{ 13, 12 }));
                return;
            }
            EXPECT_FALSE(true);
        });
    }
}
