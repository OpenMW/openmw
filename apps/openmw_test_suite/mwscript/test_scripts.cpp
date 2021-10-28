#include <gtest/gtest.h>
#include <sstream>

#include "test_utils.hpp"

namespace
{
    struct MWScriptTest : public ::testing::Test
    {
        MWScriptTest() : mErrorHandler(), mParser(mErrorHandler, mCompilerContext) {}

        std::optional<CompiledScript> compile(const std::string& scriptBody, bool shouldFail = false)
        {
            mParser.reset();
            mErrorHandler.reset();
            std::istringstream input(scriptBody);
            Compiler::Scanner scanner(mErrorHandler, input, mCompilerContext.getExtensions());
            scanner.scan(mParser);
            if(mErrorHandler.isGood())
            {
                std::vector<Interpreter::Type_Code> code;
                mParser.getCode(code);
                return CompiledScript(code, mParser.getLocals());
            }
            else if(!shouldFail)
                logErrors();
            return {};
        }

        void logErrors()
        {
            for(const auto& [error, loc] : mErrorHandler.getErrors())
            {
                std::cout << error;
                if(loc.mLine)
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
            mInterpreter.run(&script.mByteCode[0], static_cast<int>(script.mByteCode.size()), context);
        }

        void installOpcode(int code, Interpreter::Opcode0* opcode)
        {
            mInterpreter.installSegment5(code, opcode);
        }
    protected:
        void SetUp() override
        {
            Interpreter::installOpcodes(mInterpreter);
        }

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
        if(auto script = compile(sScript2))
        {
            class AddTopic : public Interpreter::Opcode0
            {
                bool& mFailed;
            public:
                AddTopic(bool& failed) : mFailed(failed) {}

                void execute(Interpreter::Runtime& runtime)
                {
                    const auto topic = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    mFailed = false;
                    EXPECT_EQ(topic, "OpenMW Unit Test");
                }
            };
            installOpcode(Compiler::Dialogue::opcodeAddTopic, new AddTopic(failed));
            TestInterpreterContext context;
            run(*script, context);
        }
        if(failed)
        {
            FAIL();
        }
    }

    TEST_F(MWScriptTest, mwscript_test_math)
    {
        if(auto script = compile(sScript3))
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
            for(int i = 1; i < 1000; ++i)
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
}