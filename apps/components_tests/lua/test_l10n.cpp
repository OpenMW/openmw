#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/files/fixedpath.hpp>
#include <components/l10n/manager.hpp>
#include <components/lua/l10n.hpp>
#include <components/lua/luastate.hpp>
#include <components/testing/util.hpp>

namespace
{
    using namespace testing;
    using namespace TestingOpenMW;

    template <typename T>
    T get(sol::state_view& lua, const std::string& luaCode)
    {
        return lua.safe_script("return " + luaCode).get<T>();
    }

    VFSTestFile invalidScript("not a script");
    VFSTestFile incorrectScript(
        "return { incorrectSection = {}, engineHandlers = { incorrectHandler = function() end } }");
    VFSTestFile emptyScript("");

    VFSTestFile test1En(R"X(
good_morning: "Good morning."
you_have_arrows: |-
  {count, plural,
    =0{You have no arrows.}
    one{You have one arrow.}
    other{You have {count} arrows.}
  }
pc_must_come: |-
  {PCGender, select,
     male {He is}
     female {She is}
     other {They are}
  } coming with us.
quest_completion: "The quest is {done, number, percent} complete."
ordinal: "You came in {num, ordinal} place."
spellout: "There {num, plural, one{is {num, spellout} thing} other{are {num, spellout} things}}."
duration: "It took {num, duration}"
numbers: "{int} and {double, number, integer} are integers, but {double} is a double"
rounding: "{value, number, :: .00}"
)X");

    VFSTestFile test1De(R"X(
good_morning: "Guten Morgen."
you_have_arrows: |-
  {count, plural,
    one{Du hast ein Pfeil.}
    other{Du hast {count} Pfeile.}
  }
"Hello {name}!": "Hallo {name}!"
)X");

    VFSTestFile test1EnUS(R"X(
currency: "You have {money, number, currency}"
)X");

    VFSTestFile test2En(R"X(
good_morning: "Morning!"
you_have_arrows: "Arrows count: {count}"
)X");

    struct LuaL10nTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            { "l10n/Test1/en.yaml", &test1En },
            { "l10n/Test1/en_US.yaml", &test1EnUS },
            { "l10n/Test1/de.yaml", &test1De },
            { "l10n/Test2/en.yaml", &test2En },
            { "l10n/Test3/en.yaml", &test1En },
            { "l10n/Test3/de.yaml", &test1De },
        });

        LuaUtil::ScriptsConfiguration mCfg;
    };

    TEST_F(LuaL10nTest, L10n)
    {
        LuaUtil::LuaState lua{ mVFS.get(), &mCfg };
        lua.protectedCall([&](LuaUtil::LuaView& view) {
            sol::state_view& l = view.sol();
            internal::CaptureStdout();
            l10n::Manager l10nManager(mVFS.get());
            l10nManager.setPreferredLocales({ "de", "en" });
            EXPECT_THAT(internal::GetCapturedStdout(), "Preferred locales: gmst de en\n");

            l["l10n"] = LuaUtil::initL10nLoader(l, &l10nManager);

            internal::CaptureStdout();
            l.safe_script("t1 = l10n('Test1')");
            EXPECT_THAT(internal::GetCapturedStdout(),
                "Language file \"l10n/Test1/de.yaml\" is enabled\n"
                "Language file \"l10n/Test1/en.yaml\" is enabled\n");

            internal::CaptureStdout();
            l.safe_script("t2 = l10n('Test2')");
            {
                std::string output = internal::GetCapturedStdout();
                EXPECT_THAT(output, HasSubstr("Language file \"l10n/Test2/en.yaml\" is enabled"));
            }

            EXPECT_EQ(get<std::string>(l, "t1('good_morning')"), "Guten Morgen.");
            EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=1})"), "Du hast ein Pfeil.");
            EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=5})"), "Du hast 5 Pfeile.");
            EXPECT_EQ(get<std::string>(l, "t1('Hello {name}!', {name='World'})"), "Hallo World!");
            EXPECT_EQ(get<std::string>(l, "t2('good_morning')"), "Morning!");
            EXPECT_EQ(get<std::string>(l, "t2('you_have_arrows', {count=3})"), "Arrows count: 3");

            internal::CaptureStdout();
            l10nManager.setPreferredLocales({ "en", "de" });
            EXPECT_THAT(internal::GetCapturedStdout(),
                "Preferred locales: gmst en de\n"
                "Language file \"l10n/Test1/en.yaml\" is enabled\n"
                "Language file \"l10n/Test1/de.yaml\" is enabled\n"
                "Language file \"l10n/Test2/en.yaml\" is enabled\n");

            EXPECT_EQ(get<std::string>(l, "t1('good_morning')"), "Good morning.");
            EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=1})"), "You have one arrow.");
            EXPECT_EQ(get<std::string>(l, "t1('you_have_arrows', {count=5})"), "You have 5 arrows.");
            EXPECT_EQ(get<std::string>(l, "t1('pc_must_come', {PCGender=\"male\"})"), "He is coming with us.");
            EXPECT_EQ(get<std::string>(l, "t1('pc_must_come', {PCGender=\"female\"})"), "She is coming with us.");
            EXPECT_EQ(get<std::string>(l, "t1('pc_must_come', {PCGender=\"blah\"})"), "They are coming with us.");
            EXPECT_EQ(get<std::string>(l, "t1('pc_must_come', {PCGender=\"other\"})"), "They are coming with us.");
            EXPECT_EQ(get<std::string>(l, "t1('quest_completion', {done=0.1})"), "The quest is 10% complete.");
            EXPECT_EQ(get<std::string>(l, "t1('quest_completion', {done=1})"), "The quest is 100% complete.");
            EXPECT_EQ(get<std::string>(l, "t1('ordinal', {num=1})"), "You came in 1st place.");
            EXPECT_EQ(get<std::string>(l, "t1('ordinal', {num=100})"), "You came in 100th place.");
            EXPECT_EQ(get<std::string>(l, "t1('spellout', {num=1})"), "There is one thing.");
            EXPECT_EQ(get<std::string>(l, "t1('spellout', {num=100})"), "There are one hundred things.");
            EXPECT_EQ(get<std::string>(l, "t1('duration', {num=100})"), "It took 1:40");
            EXPECT_EQ(get<std::string>(l, "t1('numbers', {int=123, double=123.456})"),
                "123 and 123 are integers, but 123.456 is a double");
            EXPECT_EQ(get<std::string>(l, "t1('rounding', {value=123.456789})"), "123.46");
            // Check that failed messages display the key instead of an empty string
            EXPECT_EQ(get<std::string>(l, "t1('{mismatched_braces')"), "{mismatched_braces");
            EXPECT_EQ(get<std::string>(l, "t1('{unknown_arg}')"), "{unknown_arg}");
            EXPECT_EQ(get<std::string>(l, "t1('{num, integer}', {num=1})"), "{num, integer}");
            // Doesn't give a valid currency symbol with `en`. Not that openmw is designed for real world currency.
            l10nManager.setPreferredLocales({ "en-US", "de" });
            EXPECT_EQ(get<std::string>(l, "t1('currency', {money=10000.10})"), "You have $10,000.10");
            // Note: Not defined in English localisation file, so we fall back to the German before falling back to the
            // key
            EXPECT_EQ(get<std::string>(l, "t1('Hello {name}!', {name='World'})"), "Hallo World!");
            EXPECT_EQ(get<std::string>(l, "t2('good_morning')"), "Morning!");
            EXPECT_EQ(get<std::string>(l, "t2('you_have_arrows', {count=3})"), "Arrows count: 3");

            // Test that locales with variants and country codes fall back to more generic locales
            internal::CaptureStdout();
            l10nManager.setPreferredLocales({ "en-GB-oed", "de" });
            EXPECT_THAT(internal::GetCapturedStdout(),
                "Preferred locales: gmst en_GB_OED de\n"
                "Language file \"l10n/Test1/en.yaml\" is enabled\n"
                "Language file \"l10n/Test1/de.yaml\" is enabled\n"
                "Language file \"l10n/Test2/en.yaml\" is enabled\n");
            EXPECT_EQ(get<std::string>(l, "t2('you_have_arrows', {count=3})"), "Arrows count: 3");

            // Test setting fallback language
            l.safe_script("t3 = l10n('Test3', 'de')");
            l10nManager.setPreferredLocales({ "en" });
            EXPECT_EQ(get<std::string>(l, "t3('Hello {name}!', {name='World'})"), "Hallo World!");
        });
    }
}
