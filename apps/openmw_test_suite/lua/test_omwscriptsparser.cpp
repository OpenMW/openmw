#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/lua/omwscriptsparser.hpp>

#include "testing_util.hpp"

namespace
{
    using namespace testing;

    TestFile file1(
        "#comment.lua\n"
        "\n"
        "script1.lua\n"
        "some mod/Some Script.lua"
    );
    TestFile file2(
        "#comment.lua\r\n"
        "\r\n"
        "script2.lua\r\n"
        "some other mod/Some Script.lua\r"
    );
    TestFile emptyFile("");
    TestFile invalidFile("Invalid file");

    struct OMWScriptsParserTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            {"file1.omwscripts", &file1},
            {"file2.omwscripts", &file2},
            {"empty.omwscripts", &emptyFile},
            {"invalid.lua", &file1},
            {"invalid.omwscripts", &invalidFile},
        });
    };

    TEST_F(OMWScriptsParserTest, Basic)
    {
        internal::CaptureStdout();
        std::vector<std::string> res = LuaUtil::parseOMWScriptsFiles(
            mVFS.get(), {"file2.omwscripts", "empty.omwscripts", "file1.omwscripts"});
        EXPECT_EQ(internal::GetCapturedStdout(), "");
        EXPECT_THAT(res, ElementsAre("script2.lua", "some other mod/Some Script.lua",
                                     "script1.lua", "some mod/Some Script.lua"));
    }

    TEST_F(OMWScriptsParserTest, InvalidFiles)
    {
        internal::CaptureStdout();
        std::vector<std::string> res = LuaUtil::parseOMWScriptsFiles(
            mVFS.get(), {"invalid.lua", "invalid.omwscripts"});
        EXPECT_EQ(internal::GetCapturedStdout(),
                  "Script list should have suffix '.omwscripts', got: 'invalid.lua'\n"
                  "Lua script should have suffix '.lua', got: 'Invalid file'\n");
        EXPECT_THAT(res, ElementsAre());
    }

}
