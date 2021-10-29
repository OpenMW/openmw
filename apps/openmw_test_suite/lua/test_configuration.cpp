#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/lua/configuration.hpp>

#include "testing_util.hpp"

namespace
{

    TEST(LuaConfigurationTest, ValidConfiguration)
    {
        ESM::LuaScriptsCfg cfg;
        LuaUtil::parseOMWScripts(cfg, R"X(
            # Lines starting with '#' are comments
            GLOBAL:  my_mod/#some_global_script.lua

            # Script that will be automatically attached to the player
            PLAYER :my_mod/player.lua
            CUSTOM : my_mod/some_other_script.lua
            NPC , CREATURE PLAYER : my_mod/some_other_script.lua)X");
        LuaUtil::parseOMWScripts(cfg, ":my_mod/player.LUA  \r\nCONTAINER,CUSTOM: my_mod/container.lua\r\n");

        ASSERT_EQ(cfg.mScripts.size(), 6);
        EXPECT_EQ(LuaUtil::scriptCfgToString(cfg.mScripts[0]), "GLOBAL : my_mod/#some_global_script.lua");
        EXPECT_EQ(LuaUtil::scriptCfgToString(cfg.mScripts[1]), "PLAYER : my_mod/player.lua");
        EXPECT_EQ(LuaUtil::scriptCfgToString(cfg.mScripts[2]), "CUSTOM : my_mod/some_other_script.lua");
        EXPECT_EQ(LuaUtil::scriptCfgToString(cfg.mScripts[3]), "CREATURE NPC PLAYER : my_mod/some_other_script.lua");
        EXPECT_EQ(LuaUtil::scriptCfgToString(cfg.mScripts[4]), ": my_mod/player.LUA");
        EXPECT_EQ(LuaUtil::scriptCfgToString(cfg.mScripts[5]), "CONTAINER CUSTOM : my_mod/container.lua");

        LuaUtil::ScriptsConfiguration conf;
        conf.init(std::move(cfg));
        ASSERT_EQ(conf.size(), 3);
        EXPECT_EQ(LuaUtil::scriptCfgToString(conf[0]), "GLOBAL : my_mod/#some_global_script.lua");
        // cfg.mScripts[1] is overridden by cfg.mScripts[4]
        // cfg.mScripts[2] is overridden by cfg.mScripts[3]
        EXPECT_EQ(LuaUtil::scriptCfgToString(conf[1]), "CREATURE NPC PLAYER : my_mod/some_other_script.lua");
        // cfg.mScripts[4] is removed because there are no flags
        EXPECT_EQ(LuaUtil::scriptCfgToString(conf[2]), "CONTAINER CUSTOM : my_mod/container.lua");

        cfg = ESM::LuaScriptsCfg();
        conf.init(std::move(cfg));
        ASSERT_EQ(conf.size(), 0);
    }

    TEST(LuaConfigurationTest, Errors)
    {
        ESM::LuaScriptsCfg cfg;
        EXPECT_ERROR(LuaUtil::parseOMWScripts(cfg, "GLOBAL: something"),
                     "Lua script should have suffix '.lua', got: GLOBAL: something");
        EXPECT_ERROR(LuaUtil::parseOMWScripts(cfg, "something.lua"),
                     "No flags found in: something.lua");
        EXPECT_ERROR(LuaUtil::parseOMWScripts(cfg, "GLOBAL, PLAYER: something.lua"),
                     "Global script can not have local flags");
    }

}
