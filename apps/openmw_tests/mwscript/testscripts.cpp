#include <gtest/gtest.h>

#include <array>
#include <sstream>

#include "testutils.hpp"

namespace
{
    struct MWScriptTest : public ::testing::Test
    {
        MWScriptTest()
            : mErrorHandler()
            , mParser(mErrorHandler, mCompilerContext)
        {
        }

        std::optional<CompiledScript> compile(const std::string& scriptBody, bool shouldFail = false)
        {
            mParser.reset();
            mErrorHandler.reset();
            std::istringstream input(scriptBody);
            Compiler::Scanner scanner(mErrorHandler, input, mCompilerContext.getExtensions());
            try
            {
                scanner.scan(mParser);
            }
            catch (...)
            {
                if (!shouldFail)
                    logErrors();
                throw;
            }
            if (mErrorHandler.isGood())
                return CompiledScript(mParser.getProgram(), mParser.getLocals());
            else if (!shouldFail)
                logErrors();
            return {};
        }

        void logErrors()
        {
            for (const auto& [error, loc] : mErrorHandler.getErrors())
            {
                std::cout << error;
                if (loc.mLine)
                    std::cout << " at line" << loc.mLine << " column " << loc.mColumn << " (" << loc.mLiteral << ")";
                std::cout << "\n";
            }
        }

        void registerExtensions()
        {
            Compiler::registerExtensions(mExtensions);
            mCompilerContext.setExtensions(&mExtensions);
        }

        void run(const CompiledScript& script, TestInterpreterContext& context)
        {
            mInterpreter.run(script.mProgram, context);
        }

        template <typename T, typename... TArgs>
        void installOpcode(int code, TArgs&&... args)
        {
            mInterpreter.installSegment5<T>(code, std::forward<TArgs&&>(args)...);
        }

    protected:
        void SetUp() override { Interpreter::installOpcodes(mInterpreter); }

        void TearDown() override {}

    private:
        TestErrorHandler mErrorHandler;
        TestCompilerContext mCompilerContext;
        Compiler::FileParser mParser;
        Compiler::Extensions mExtensions;
        Interpreter::Interpreter mInterpreter;
    };

    const std::string sScript1 = R"mwscript(Begin basic_logic
; Comment
short one
short two

set one to two

if ( one == two )
    set one to 1
elseif ( two == 1 )
    set one to 2
else
    set one to 3
endif

while ( one < two )
    set one to ( one + 1 )
endwhile

End)mwscript";

    const std::string sScript2 = R"mwscript(Begin addtopic

AddTopic "OpenMW Unit Test"

End)mwscript";

    const std::string sScript3 = R"mwscript(Begin math

short a
short b
short c
short d
short e

set b to ( a + 1 )
set c to ( a - 1 )
set d to ( b * c )
set e to ( d / a )

End)mwscript";

    // https://forum.openmw.org/viewtopic.php?f=6&t=2262
    const std::string sScript4 = R"mwscript(Begin scripting_once_again

player -> addSpell "fire_bite", 645

PositionCell "Rabenfels, Taverne" 4480.000 3968.000 15820.000 0

End)mwscript";

    const std::string sScript5 = R"mwscript(Begin messagebox_format_script

float fVal

set fVal to 12.34

MessageBox "hello world"
MessageBox "%.0f" fVal
MessageBox "a %03.0f b" fVal
MessageBox "%+04.0f" fVal
MessageBox "%+4.0f" fVal
MessageBox "%+ 4.0f" fVal
MessageBox "%0+ 4.0f" fVal
MessageBox "%0+ #4.0f" fVal
MessageBox "%- 5.0f" fVal

MessageBox "%g" fVal
MessageBox "%.3g" fVal
MessageBox "%.5g" fVal
MessageBox "%#.5g" fVal
MessageBox "%-5g" fVal
MessageBox "%- 5g" fVal

End)mwscript";

    const std::string sIssue587 = R"mwscript(Begin stalresetScript

End stalreset Script)mwscript";

    const std::string sIssue677 = R"mwscript(Begin _ase_dtree_dtree-owls

End)mwscript";

    const std::string sIssue685 = R"mwscript(Begin issue685

Choice: "Sicher. Hier, nehmt." 1 "Nein, ich denke nicht. Tut mir Leid." 2
StartScript GetPCGold

End)mwscript";

    const std::string sIssue694 = R"mwscript(Begin issue694

float timer

if ( timer < .1 )
endif

End)mwscript";

    const std::string sIssue1062 = R"mwscript(Begin issue1026

short end

End)mwscript";

    const std::string sIssue1430 = R"mwscript(Begin issue1430

short var
If ( menumode == 1 )
    Player->AddItem "fur_boots", 1
    Player->Equip "iron battle axe", 1
    player->addspell "fire bite", 645
    player->additem "ring_keley", 1,
endif

End)mwscript";

    const std::string sIssue1593 = R"mwscript(Begin changeWater_-550_400

End)mwscript";

    const std::string sIssue1730 = R"mwscript(Begin 4LOM_Corprusarium_Guards

End)mwscript";

    const std::string sIssue1767 = R"mwscript(Begin issue1767

player->GetPcRank "temple"

End)mwscript";

    const std::string sIssue2185 = R"mwscript(Begin issue2185

short a
short b
short eq
short gt
short lt
short ne

set eq to 0
if ( a == b )
    set eq to ( eq + 1 )
endif
if ( a = = b )
    set eq to ( eq + 1 )
endif

set gt to 0
if ( a > b )
    set gt to ( gt + 1 )
endif
if ( a > = b )
    set gt to ( gt + 1 )
endif

set lt to 0
if ( a < b )
    set lt to ( lt + 1 )
endif
if ( a < = b )
    set lt to ( lt + 1 )
endif

set ne to 0
if ( a != b )
    set ne to ( ne + 1 )
endif
if ( a ! = b )
    set ne to ( ne + 1 )
endif

End)mwscript";

    const std::string sIssue2206 = R"mwscript(Begin issue2206

Choice ."Sklavin kaufen." 1 "Lebt wohl." 2
Choice Choice "Insister pour qu’il vous réponde." 6 "Le prier de vous accorder un peu de son temps." 6 " Le menacer de révéler qu'il prélève sa part sur les bénéfices de la mine d’ébonite." 7

End)mwscript";

    const std::string sIssue2207 = R"mwscript(Begin issue2207

PositionCell -35 –473 -248 0 "Skaal-Dorf, Die Große Halle"

End)mwscript";

    const std::string sIssue2794 = R"mwscript(Begin issue2794

if ( player->"getlevel" == 1 )
    ; do something
endif

End)mwscript";

    const std::string sIssue2830 = R"mwscript(Begin issue2830

AddItem "if" 1
AddItem "endif" 1
GetItemCount "begin"

End)mwscript";

    const std::string sIssue2991 = R"mwscript(Begin issue2991

MessageBox "OnActivate"
messagebox "messagebox"
messagebox "if"
messagebox "tcl"

End)mwscript";

    const std::string sIssue3006 = R"mwscript(Begin issue3006

short a

if ( a == 1 )
    set a to 2
else set a to 3
endif

End)mwscript";

    const std::string sIssue3725 = R"mwscript(Begin issue3725

onactivate

if onactivate
    ; do something
endif

End)mwscript";

    const std::string sIssue3744 = R"mwscript(Begin issue3744

short a
short b
short c

set c to 0

if ( a => b )
    set c to ( c + 1 )
endif
if ( a =< b )
    set c to ( c + 1 )
endif
if ( a = b )
    set c to ( c + 1 )
endif
if ( a == b )
    set c to ( c + 1 )
endif

End)mwscript";

    const std::string sIssue3836 = R"mwscript(Begin issue3836

MessageBox " Membership Level:         %.0f
Account Balance:           %.0f
Your Gold:                   %.0f
Interest Rate:              %.3f
Service Charge Rate:      %.3f
Total Service Charges:    %.0f
Total Interest Earned:     %.0f " Membership BankAccount YourGold InterestRate ServiceRate TotalServiceCharges TotalInterestEarned

End)mwscript";

    const std::string sIssue3846 = R"mwscript(Begin issue3846

Addtopic -spells...
Addtopic -magicka...

player->PositionCell, -97274, -94273, 8064, -12,-12

End)mwscript";

    const std::string sIssue4061 = R"mwscript(Begin 01_Rz_neuvazhay-koryto2

End)mwscript";

    const std::string sIssue4451 = R"mwscript(Begin, GlassDisplayScript

;[Script body]

End, GlassDisplayScript)mwscript";

    const std::string sIssue4597 = R"mwscript(Begin issue4597

short a
short b
short c
short d

set c to 0
set d to 0

if ( a <> b )
    set c to ( c + 1 )
endif
if ( a << b )
    set c to ( c + 1 )
endif
if ( a < b )
    set c to ( c + 1 )
endif

if ( a >< b )
    set d to ( d + 1 )
endif
if ( a >> b )
    set d to ( d + 1 )
endif
if ( a > b )
    set d to ( d + 1 )
endif

End)mwscript";

    const std::string sIssue4598 = R"mwscript(Begin issue4598

StartScript kal_S_Pub_Jejubãr_Faraminos

End)mwscript";

    const std::string sIssue4803 = R"mwscript(
--
+-Begin issue4803

End)mwscript";

    const std::string sIssue4867 = R"mwscript(Begin issue4867

float PcMagickaMult :  The gameplay setting fPcBaseMagickaMult - 1.0000

End)mwscript";

    const std::string sIssue4888 = R"mwscript(Begin issue4888

if (player->GameHour == 10)
set player->GameHour to 20
endif

End)mwscript";

    const std::string sIssue4996 = R"mwscript(---Begin issue4996

player-> SetPos, Z, myZ + 50

End)mwscript";

    const std::string sIssue5087 = R"mwscript(Begin Begin

player->sethealth 0
stopscript Begin

End Begin)mwscript";

    const std::string sIssue5097 = R"mwscript(Begin issue5097

setscale "0.3"

End)mwscript";

    const std::string sIssue5345 = R"mwscript(Begin issue5345

StartScript DN_MinionDrain_s"

End)mwscript";

    const std::string sIssue6066 = R"mwscript(Begin issue6066
addtopic "return"

End)mwscript";

    const std::string sIssue6282 = R"mwscript(Begin 11AA_LauraScript7.5

End)mwscript";

    const std::string sIssue6363 = R"mwscript(Begin issue6363

short 1

if ( "1" == 1 )
    PositionCell 0 1 2 3 4 5 "Morrowland"
endif

set 1 to 42

End)mwscript";

    const std::string sIssue6380 = R"mwscript(,Begin,issue6380,

,short,a

,set,a,to,,,,(a,+1)

messagebox,"this is a %g",a

,End,)mwscript";

    const std::string sIssue6807 = R"mwscript(---Begin issue6807

short a

----------------------
+++++++++++
***************
/////////////////////
?????????
@@@@@@@@
~~~~~~~~~~~~~~~~~~

set a to 1

;-------------------------------------------------------------------------------------------------------------------------------------------------------------
; Collision Detection Check
-------------------------------------------------------------------------------------------------------------------------------------------------------------

-+'\/.,><$@---!=\/?--------(){}------ show a

( GetDisabled == 1 )
GetDisabled == 1

End)mwscript";

    TEST_F(MWScriptTest, mwscript_test_invalid)
    {
        EXPECT_THROW(compile("this is not a valid script", true), Compiler::SourceException);
    }

    TEST_F(MWScriptTest, mwscript_test_compilation)
    {
        EXPECT_FALSE(!compile(sScript1));
    }

    TEST_F(MWScriptTest, mwscript_test_no_extensions)
    {
        EXPECT_THROW(compile(sScript2, true), Compiler::SourceException);
    }

    TEST_F(MWScriptTest, mwscript_test_function)
    {
        registerExtensions();
        bool failed = true;
        if (const auto script = compile(sScript2))
        {
            class AddTopic : public Interpreter::Opcode0
            {
                bool& mFailed;

            public:
                AddTopic(bool& failed)
                    : mFailed(failed)
                {
                }

                void execute(Interpreter::Runtime& runtime)
                {
                    const auto topic = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    mFailed = false;
                    EXPECT_EQ(topic, "OpenMW Unit Test");
                }
            };
            installOpcode<AddTopic>(Compiler::Dialogue::opcodeAddTopic, failed);
            TestInterpreterContext context;
            run(*script, context);
        }
        if (failed)
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_math)
    {
        if (const auto script = compile(sScript3))
        {
            struct Algorithm
            {
                int a;
                int b;
                int c;
                int d;
                int e;

                void run(int input)
                {
                    a = input;
                    b = a + 1;
                    c = a - 1;
                    d = b * c;
                    e = d / a;
                }

                void test(const TestInterpreterContext& context) const
                {
                    EXPECT_EQ(a, context.getLocalShort(0));
                    EXPECT_EQ(b, context.getLocalShort(1));
                    EXPECT_EQ(c, context.getLocalShort(2));
                    EXPECT_EQ(d, context.getLocalShort(3));
                    EXPECT_EQ(e, context.getLocalShort(4));
                }
            } algorithm;
            TestInterpreterContext context;
            for (int i = 1; i < 1000; ++i)
            {
                context.setLocalShort(0, i);
                run(*script, context);
                algorithm.run(i);
                algorithm.test(context);
            }
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_forum_thread)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sScript4));
    }

    TEST_F(MWScriptTest, mwscript_test_messagebox_format)
    {
        if (const auto script = compile(sScript5))
        {
            TestInterpreterContext context;
            run(*script, context);
            using std::string_view_literals::operator""sv;
            constexpr std::array expected{
                "hello world"sv,
                "12"sv,
                "a 012 b"sv,
                "+012"sv,
                " +12"sv,
                " +12"sv,
                "+012"sv,
                "+12."sv,
                " 12  "sv,

                "12.34"sv,
                "12.3"sv,
                "12.34"sv,
                "12.340"sv,
                "12.34"sv,
                " 12.34"sv,
            };
            for (std::size_t i = 0; i < context.getMessages().size(); i++)
            {
                std::string_view message = context.getMessages()[i];
                EXPECT_EQ(expected.at(i), message);
            }
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_587)
    {
        EXPECT_FALSE(!compile(sIssue587));
    }

    TEST_F(MWScriptTest, mwscript_test_677)
    {
        EXPECT_FALSE(!compile(sIssue677));
    }

    TEST_F(MWScriptTest, mwscript_test_685)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue685));
    }

    TEST_F(MWScriptTest, mwscript_test_694)
    {
        EXPECT_FALSE(!compile(sIssue694));
    }

    TEST_F(MWScriptTest, mwscript_test_1062)
    {
        if (const auto script = compile(sIssue1062))
        {
            EXPECT_EQ(script->mLocals.getIndex("end"), 0);
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_1430)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue1430));
    }

    TEST_F(MWScriptTest, mwscript_test_1593)
    {
        EXPECT_FALSE(!compile(sIssue1593));
    }

    TEST_F(MWScriptTest, mwscript_test_1730)
    {
        EXPECT_FALSE(!compile(sIssue1730));
    }

    TEST_F(MWScriptTest, mwscript_test_1767)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue1767));
    }

    TEST_F(MWScriptTest, mwscript_test_2185)
    {
        if (const auto script = compile(sIssue2185))
        {
            TestInterpreterContext context;
            for (int a = 0; a < 100; ++a)
            {
                for (int b = 0; b < 100; ++b)
                {
                    context.setLocalShort(0, a);
                    context.setLocalShort(1, b);
                    run(*script, context);
                    EXPECT_EQ(context.getLocalShort(2), a == b ? 2 : 0);
                    EXPECT_EQ(context.getLocalShort(3), a > b ? 2 : 0);
                    EXPECT_EQ(context.getLocalShort(4), a < b ? 2 : 0);
                    EXPECT_EQ(context.getLocalShort(5), a != b ? 2 : 0);
                }
            }
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_2206)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue2206));
    }

    TEST_F(MWScriptTest, mwscript_test_2207)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue2207));
    }

    TEST_F(MWScriptTest, mwscript_test_2794)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue2794));
    }

    TEST_F(MWScriptTest, mwscript_test_2830)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue2830));
    }

    TEST_F(MWScriptTest, mwscript_test_2991)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue2991));
    }

    TEST_F(MWScriptTest, mwscript_test_3006)
    {
        if (const auto script = compile(sIssue3006))
        {
            TestInterpreterContext context;
            context.setLocalShort(0, 0);
            run(*script, context);
            EXPECT_EQ(context.getLocalShort(0), 0);
            context.setLocalShort(0, 1);
            run(*script, context);
            EXPECT_EQ(context.getLocalShort(0), 2);
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_3725)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue3725));
    }

    TEST_F(MWScriptTest, mwscript_test_3744)
    {
        if (const auto script = compile(sIssue3744))
        {
            TestInterpreterContext context;
            for (int a = 0; a < 100; ++a)
            {
                for (int b = 0; b < 100; ++b)
                {
                    context.setLocalShort(0, a);
                    context.setLocalShort(1, b);
                    run(*script, context);
                    EXPECT_EQ(context.getLocalShort(2), a == b ? 4 : 0);
                }
            }
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_3836)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue3836));
    }

    TEST_F(MWScriptTest, mwscript_test_3846)
    {
        registerExtensions();
        if (const auto script = compile(sIssue3846))
        {
            std::vector<std::string> topics = { "-spells...", "-magicka..." };
            class AddTopic : public Interpreter::Opcode0
            {
                std::vector<std::string>& mTopics;

            public:
                AddTopic(std::vector<std::string>& topics)
                    : mTopics(topics)
                {
                }

                void execute(Interpreter::Runtime& runtime)
                {
                    const auto topic = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    EXPECT_EQ(topic, mTopics[0]);
                    mTopics.erase(mTopics.begin());
                }
            };
            class PositionCell : public Interpreter::Opcode0
            {
            public:
                void execute(Interpreter::Runtime& runtime)
                {
                    std::string_view target = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    Interpreter::Type_Float x = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float y = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float z = runtime[0].mFloat;
                    runtime.pop();
                    Interpreter::Type_Float zRot = runtime[0].mFloat;
                    runtime.pop();
                    std::string_view cellID = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    EXPECT_EQ(target, "player");
                    EXPECT_EQ(x, -97274);
                    EXPECT_EQ(y, -94273);
                    EXPECT_EQ(z, 8064);
                    EXPECT_EQ(zRot, -12);
                    EXPECT_EQ(cellID, "-12");
                }
            };
            installOpcode<AddTopic>(Compiler::Dialogue::opcodeAddTopic, topics);
            installOpcode<PositionCell>(Compiler::Transformation::opcodePositionCellExplicit);
            TestInterpreterContext context;
            run(*script, context);
            EXPECT_TRUE(topics.empty());
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_4061)
    {
        EXPECT_FALSE(!compile(sIssue4061));
    }

    TEST_F(MWScriptTest, mwscript_test_4451)
    {
        EXPECT_FALSE(!compile(sIssue4451));
    }

    TEST_F(MWScriptTest, mwscript_test_4597)
    {
        if (const auto script = compile(sIssue4597))
        {
            TestInterpreterContext context;
            for (int a = 0; a < 100; ++a)
            {
                for (int b = 0; b < 100; ++b)
                {
                    context.setLocalShort(0, a);
                    context.setLocalShort(1, b);
                    run(*script, context);
                    EXPECT_EQ(context.getLocalShort(2), a < b ? 3 : 0);
                    EXPECT_EQ(context.getLocalShort(3), a > b ? 3 : 0);
                }
            }
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_4598)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue4598));
    }

    TEST_F(MWScriptTest, mwscript_test_4803)
    {
        EXPECT_FALSE(!compile(sIssue4803));
    }

    TEST_F(MWScriptTest, mwscript_test_4867)
    {
        EXPECT_FALSE(!compile(sIssue4867));
    }

    TEST_F(MWScriptTest, mwscript_test_4888)
    {
        EXPECT_FALSE(!compile(sIssue4888));
    }

    TEST_F(MWScriptTest, mwscript_test_4996)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue4996));
    }

    TEST_F(MWScriptTest, mwscript_test_5087)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue5087));
    }

    TEST_F(MWScriptTest, mwscript_test_5097)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue5097));
    }

    TEST_F(MWScriptTest, mwscript_test_5345)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue5345));
    }

    TEST_F(MWScriptTest, mwscript_test_6066)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue6066));
    }

    TEST_F(MWScriptTest, mwscript_test_6282)
    {
        EXPECT_FALSE(!compile(sIssue6282));
    }

    TEST_F(MWScriptTest, mwscript_test_6363)
    {
        registerExtensions();
        if (const auto script = compile(sIssue6363))
        {
            class PositionCell : public Interpreter::Opcode0
            {
                bool& mRan;

            public:
                PositionCell(bool& ran)
                    : mRan(ran)
                {
                }

                void execute(Interpreter::Runtime& runtime) { mRan = true; }
            };
            bool ran = false;
            installOpcode<PositionCell>(Compiler::Transformation::opcodePositionCell, ran);
            TestInterpreterContext context;
            context.setLocalShort(0, 0);
            run(*script, context);
            EXPECT_FALSE(ran);
            ran = false;
            context.setLocalShort(0, 1);
            run(*script, context);
            EXPECT_TRUE(ran);
        }
        else
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_6380)
    {
        EXPECT_FALSE(!compile(sIssue6380));
    }

    TEST_F(MWScriptTest, mwscript_test_6807)
    {
        registerExtensions();
        EXPECT_FALSE(!compile(sIssue6807));
    }
}
