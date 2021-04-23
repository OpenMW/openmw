#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/esm/luascripts.hpp>

#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "testing_util.hpp"

namespace
{
    using namespace testing;

    TestFile invalidScript("not a script");
    TestFile incorrectScript("return { incorrectSection = {}, engineHandlers = { incorrectHandler = function() end } }");
    TestFile emptyScript("");
    
    TestFile testScript(R"X(
return {
    engineHandlers = { onUpdate = function(dt) print(' update ' .. tostring(dt)) end },
    eventHandlers = {
        Event1 = function(eventData) print(' event1 ' .. tostring(eventData.x)) end,
        Event2 = function(eventData) print(' event2 ' .. tostring(eventData.x)) end,
        Print = function() print('print') end
    }
}
)X");

    TestFile stopEventScript(R"X(
return {
    eventHandlers = {
        Event1 = function(eventData)
            print(' event1 ' .. tostring(eventData.x))
            return eventData.x >= 1
        end
    }
}
)X");

    TestFile loadSaveScript(R"X(
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

    TestFile interfaceScript(R"X(
return {
    interfaceName = "TestInterface",
    interface = {
        fn = function(x) print('FN', x) end,
        value = 3.5
    },
}
)X");

    TestFile overrideInterfaceScript(R"X(
local old = require('openmw.interfaces').TestInterface
return {
    interfaceName = "TestInterface",
    interface = {
        fn = function(x)
            print('NEW FN', x)
            old.fn(x)
        end,
        value = old.value + 1
    },
}
)X");

    TestFile useInterfaceScript(R"X(
local interfaces = require('openmw.interfaces')
return {
    engineHandlers = {
        onUpdate = function()
            interfaces.TestInterface.fn(interfaces.TestInterface.value)
        end,
    },
}
)X");

    struct LuaScriptsContainerTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            {"invalid.lua", &invalidScript},
            {"incorrect.lua", &incorrectScript},
            {"empty.lua", &emptyScript},
            {"test1.lua", &testScript},
            {"test2.lua", &testScript},
            {"stopEvent.lua", &stopEventScript},
            {"loadSave1.lua", &loadSaveScript},
            {"loadSave2.lua", &loadSaveScript},
            {"testInterface.lua", &interfaceScript},
            {"overrideInterface.lua", &overrideInterfaceScript},
            {"useInterface.lua", &useInterfaceScript},
        });

        LuaUtil::LuaState mLua{mVFS.get()};
    };

    TEST_F(LuaScriptsContainerTest, VerifyStructure)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        {
            testing::internal::CaptureStdout();
            EXPECT_FALSE(scripts.addNewScript("invalid.lua"));
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_THAT(output, HasSubstr("Can't start Test[invalid.lua]"));
        }
        {
            testing::internal::CaptureStdout();
            EXPECT_TRUE(scripts.addNewScript("incorrect.lua"));
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_THAT(output, HasSubstr("Not supported handler 'incorrectHandler' in Test[incorrect.lua]"));
            EXPECT_THAT(output, HasSubstr("Not supported section 'incorrectSection' in Test[incorrect.lua]"));
        }
        {
            testing::internal::CaptureStdout();
            EXPECT_TRUE(scripts.addNewScript("empty.lua"));
            EXPECT_FALSE(scripts.addNewScript("empty.lua"));  // already present
            EXPECT_EQ(internal::GetCapturedStdout(), "");
        }
    }

    TEST_F(LuaScriptsContainerTest, CallHandler)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        testing::internal::CaptureStdout();
        EXPECT_TRUE(scripts.addNewScript("test1.lua"));
        EXPECT_TRUE(scripts.addNewScript("stopEvent.lua"));
        EXPECT_TRUE(scripts.addNewScript("test2.lua"));
        scripts.update(1.5f);
        EXPECT_EQ(internal::GetCapturedStdout(), "Test[test1.lua]:\t update 1.5\n"
                                                 "Test[test2.lua]:\t update 1.5\n");
    }

    TEST_F(LuaScriptsContainerTest, CallEvent)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        EXPECT_TRUE(scripts.addNewScript("test1.lua"));
        EXPECT_TRUE(scripts.addNewScript("stopEvent.lua"));
        EXPECT_TRUE(scripts.addNewScript("test2.lua"));

        std::string X0 = LuaUtil::serialize(mLua.sol().create_table_with("x", 0.5));
        std::string X1 = LuaUtil::serialize(mLua.sol().create_table_with("x", 1.5));

        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("SomeEvent", X1);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test has received event 'SomeEvent', but there are no handlers for this event\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event1", X1);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test2.lua]:\t event1 1.5\n"
                      "Test[stopEvent.lua]:\t event1 1.5\n"
                      "Test[test1.lua]:\t event1 1.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event2", X1);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test2.lua]:\t event2 1.5\n"
                      "Test[test1.lua]:\t event2 1.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event1", X0);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test2.lua]:\t event1 0.5\n"
                      "Test[stopEvent.lua]:\t event1 0.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts.receiveEvent("Event2", X0);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test2.lua]:\t event2 0.5\n"
                      "Test[test1.lua]:\t event2 0.5\n");
        }
    }

    TEST_F(LuaScriptsContainerTest, RemoveScript)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        EXPECT_TRUE(scripts.addNewScript("test1.lua"));
        EXPECT_TRUE(scripts.addNewScript("stopEvent.lua"));
        EXPECT_TRUE(scripts.addNewScript("test2.lua"));
        std::string X = LuaUtil::serialize(mLua.sol().create_table_with("x", 0.5));

        {
            testing::internal::CaptureStdout();
            scripts.update(1.5f);
            scripts.receiveEvent("Event1", X);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test1.lua]:\t update 1.5\n"
                      "Test[test2.lua]:\t update 1.5\n"
                      "Test[test2.lua]:\t event1 0.5\n"
                      "Test[stopEvent.lua]:\t event1 0.5\n");
        }
        {
            testing::internal::CaptureStdout();
            EXPECT_TRUE(scripts.removeScript("stopEvent.lua"));
            EXPECT_FALSE(scripts.removeScript("stopEvent.lua"));  // already removed
            scripts.update(1.5f);
            scripts.receiveEvent("Event1", X);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test1.lua]:\t update 1.5\n"
                      "Test[test2.lua]:\t update 1.5\n"
                      "Test[test2.lua]:\t event1 0.5\n"
                      "Test[test1.lua]:\t event1 0.5\n");
        }
        {
            testing::internal::CaptureStdout();
            EXPECT_TRUE(scripts.removeScript("test1.lua"));
            scripts.update(1.5f);
            scripts.receiveEvent("Event1", X);
            EXPECT_EQ(internal::GetCapturedStdout(),
                      "Test[test2.lua]:\t update 1.5\n"
                      "Test[test2.lua]:\t event1 0.5\n");
        }
    }

    TEST_F(LuaScriptsContainerTest, Interface)
    {
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        testing::internal::CaptureStdout();
        EXPECT_TRUE(scripts.addNewScript("testInterface.lua"));
        EXPECT_TRUE(scripts.addNewScript("overrideInterface.lua"));
        EXPECT_TRUE(scripts.addNewScript("useInterface.lua"));
        scripts.update(1.5f);
        EXPECT_TRUE(scripts.removeScript("overrideInterface.lua"));
        scripts.update(1.5f);
        EXPECT_EQ(internal::GetCapturedStdout(),
            "Test[overrideInterface.lua]:\tNEW FN\t4.5\n"
            "Test[testInterface.lua]:\tFN\t4.5\n"
            "Test[testInterface.lua]:\tFN\t3.5\n");
    }

    TEST_F(LuaScriptsContainerTest, LoadSave)
    {
        LuaUtil::ScriptsContainer scripts1(&mLua, "Test");
        LuaUtil::ScriptsContainer scripts2(&mLua, "Test");
        LuaUtil::ScriptsContainer scripts3(&mLua, "Test");

        EXPECT_TRUE(scripts1.addNewScript("loadSave1.lua"));
        EXPECT_TRUE(scripts1.addNewScript("test1.lua"));
        EXPECT_TRUE(scripts1.addNewScript("loadSave2.lua"));

        EXPECT_TRUE(scripts3.addNewScript("test2.lua"));
        EXPECT_TRUE(scripts3.addNewScript("loadSave2.lua"));

        scripts1.receiveEvent("Set", LuaUtil::serialize(mLua.sol().create_table_with(
            "n", 1,
            "x", 0.5,
            "y", 3.5)));
        scripts1.receiveEvent("Set", LuaUtil::serialize(mLua.sol().create_table_with(
            "n", 2,
            "x", 2.5,
            "y", 1.5)));

        ESM::LuaScripts data;
        scripts1.save(data);
        scripts2.load(data, true);
        scripts3.load(data, false);

        {
            testing::internal::CaptureStdout();
            scripts2.receiveEvent("Print", "");
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[loadSave2.lua]:\t0.5\t3.5\n"
                "Test[test1.lua]:\tprint\n"
                "Test[loadSave1.lua]:\t2.5\t1.5\n");
        }
        {
            testing::internal::CaptureStdout();
            scripts3.receiveEvent("Print", "");
            EXPECT_EQ(internal::GetCapturedStdout(),
                "Test[loadSave2.lua]:\t0.5\t3.5\n"
                "Test[test2.lua]:\tprint\n");
        }
    }

    TEST_F(LuaScriptsContainerTest, Timers)
    {
        using TimeUnit = LuaUtil::ScriptsContainer::TimeUnit;
        LuaUtil::ScriptsContainer scripts(&mLua, "Test");
        EXPECT_TRUE(scripts.addNewScript("test1.lua"));
        EXPECT_TRUE(scripts.addNewScript("test2.lua"));

        int counter1 = 0, counter2 = 0, counter3 = 0, counter4 = 0;
        sol::function fn1 = sol::make_object(mLua.sol(), [&]() { counter1++; });
        sol::function fn2 = sol::make_object(mLua.sol(), [&]() { counter2++; });
        sol::function fn3 = sol::make_object(mLua.sol(), [&](int d) { counter3 += d; });
        sol::function fn4 = sol::make_object(mLua.sol(), [&](int d) { counter4 += d; });

        scripts.registerTimerCallback("test1.lua", "A", fn3);
        scripts.registerTimerCallback("test1.lua", "B", fn4);
        scripts.registerTimerCallback("test2.lua", "B", fn3);
        scripts.registerTimerCallback("test2.lua", "A", fn4);

        scripts.processTimers(1, 2);

        scripts.setupSerializableTimer(TimeUnit::SECONDS, 10, "test1.lua", "B", sol::make_object(mLua.sol(), 3));
        scripts.setupSerializableTimer(TimeUnit::HOURS, 10, "test2.lua", "B", sol::make_object(mLua.sol(), 4));
        scripts.setupSerializableTimer(TimeUnit::SECONDS, 5, "test1.lua", "A", sol::make_object(mLua.sol(), 1));
        scripts.setupSerializableTimer(TimeUnit::HOURS, 5, "test2.lua", "A", sol::make_object(mLua.sol(), 2));
        scripts.setupSerializableTimer(TimeUnit::SECONDS, 15, "test1.lua", "A", sol::make_object(mLua.sol(), 10));
        scripts.setupSerializableTimer(TimeUnit::SECONDS, 15, "test1.lua", "B", sol::make_object(mLua.sol(), 20));

        scripts.setupUnsavableTimer(TimeUnit::SECONDS, 10, "test2.lua", fn2);
        scripts.setupUnsavableTimer(TimeUnit::HOURS, 10, "test1.lua", fn2);
        scripts.setupUnsavableTimer(TimeUnit::SECONDS, 5, "test2.lua", fn1);
        scripts.setupUnsavableTimer(TimeUnit::HOURS, 5, "test1.lua", fn1);
        scripts.setupUnsavableTimer(TimeUnit::SECONDS, 15, "test2.lua", fn1);

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

        ESM::LuaScripts data;
        scripts.save(data);
        scripts.load(data, true);
        scripts.registerTimerCallback("test1.lua", "B", fn4);

        testing::internal::CaptureStdout();
        scripts.processTimers(20, 20);
        EXPECT_EQ(internal::GetCapturedStdout(), "Test[test1.lua] callTimer failed: Callback 'A' doesn't exist\n");

        EXPECT_EQ(counter1, 2);
        EXPECT_EQ(counter2, 2);
        EXPECT_EQ(counter3, 5);
        EXPECT_EQ(counter4, 25);
    }

}
