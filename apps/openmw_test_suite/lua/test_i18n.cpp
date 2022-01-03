#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <components/files/fixedpath.hpp>

#include <components/lua/luastate.hpp>
#include <components/lua/i18n.hpp>

#include "testing_util.hpp"

namespace
{
    using namespace testing;

    TestFile invalidScript("not a script");
    TestFile incorrectScript("return { incorrectSection = {}, engineHandlers = { incorrectHandler = function() end } }");
    TestFile emptyScript("");
    
    TestFile test1En(R"X(
return {
    good_morning = "Good morning.",
    you_have_arrows = {
      one = "You have one arrow.",
      other = "You have %{count} arrows.",
    },
}
)X");

    TestFile test1De(R"X(
return {
    good_morning = "Guten Morgen.",
    you_have_arrows = {
      one = "Du hast ein Pfeil.",
      other = "Du hast %{count} Pfeile.",
    },
    ["Hello %{name}!"] = "Hallo %{name}!",
}
)X");

TestFile test2En(R"X(
return {
    good_morning = "Morning!",
    you_have_arrows = "Arrows count: %{count}",
}
)X");

    TestFile invalidTest2De(R"X(
require('math')
return {}
)X");

    struct LuaI18nTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            {"i18n/Test1/en.lua", &test1En},
            {"i18n/Test1/de.lua", &test1De},
            {"i18n/Test2/en.lua", &test2En},
            {"i18n/Test2/de.lua", &invalidTest2De},
        });

        LuaUtil::ScriptsConfiguration mCfg;
        std::string mLibsPath = (Files::TargetPathType("openmw_test_suite").getLocalPath() / "resources" / "lua_libs").string();
    };

    TEST_F(LuaI18nTest, I18n)
    {
        internal::CaptureStdout();
        LuaUtil::LuaState lua{mVFS.get(), &mCfg};
        sol::state& l = lua.sol();
        LuaUtil::I18nManager i18n(mVFS.get(), &lua);
        lua.addInternalLibSearchPath(mLibsPath);
        i18n.init();
        i18n.setPreferredLanguages({"de", "en"});
        EXPECT_THAT(internal::GetCapturedStdout(), "I18n preferred languages: de en\n");

        internal::CaptureStdout();
        l["t1"] = i18n.getContext("Test1");
        EXPECT_THAT(internal::GetCapturedStdout(), "Language file \"i18n/Test1/de.lua\" is enabled\n");

        internal::CaptureStdout();
        l["t2"] = i18n.getContext("Test2");
        {
            std::string output = internal::GetCapturedStdout();
            EXPECT_THAT(output, HasSubstr("Can not load i18n/Test2/de.lua"));
            EXPECT_THAT(output, HasSubstr("Language file \"i18n/Test2/en.lua\" is enabled"));
        }

        EXPECT_EQ(get<std::string>(l, "t1('good_morning')"), "Guten Morgen.");
        EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=1})"), "Du hast ein Pfeil.");
        EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=5})"), "Du hast 5 Pfeile.");
        EXPECT_EQ(get<std::string>(l, "t1('Hello %{name}!', {name='World'})"), "Hallo World!");
        EXPECT_EQ(get<std::string>(l, "t2('good_morning')"), "Morning!");
        EXPECT_EQ(get<std::string>(l, "t2('you_have_arrows', {count=3})"), "Arrows count: 3");

        internal::CaptureStdout();
        i18n.setPreferredLanguages({"en", "de"});
        EXPECT_THAT(internal::GetCapturedStdout(),
            "I18n preferred languages: en de\n"
            "Language file \"i18n/Test1/en.lua\" is enabled\n"
            "Language file \"i18n/Test2/en.lua\" is enabled\n");

        EXPECT_EQ(get<std::string>(l, "t1('good_morning')"), "Good morning.");
        EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=1})"), "You have one arrow.");
        EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=5})"), "You have 5 arrows.");
        EXPECT_EQ(get<std::string>(l, "t1('Hello %{name}!', {name='World'})"), "Hello World!");
        EXPECT_EQ(get<std::string>(l, "t2('good_morning')"), "Morning!");
        EXPECT_EQ(get<std::string>(l, "t2('you_have_arrows', {count=3})"), "Arrows count: 3");
    }

}
