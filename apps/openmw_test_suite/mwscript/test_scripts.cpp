#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <components/compiler/context.hpp>
#include <components/compiler/errorhandler.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/scanner.hpp>

#include <components/interpreter/interpreter.hpp>

namespace
{
    class TestCompilerContext : public Compiler::Context
    {
    public:
        bool canDeclareLocals() const override { return true; }
        char getGlobalType(const std::string& name) const override { return ' '; }
        std::pair<char, bool> getMemberType(const std::string& name, const std::string& id) const override { return {' ', false}; }
        bool isId(const std::string& name) const override { return false; }
        bool isJournalId(const std::string& name) const override { return false; }
    };

    class TestErrorHandler : public Compiler::ErrorHandler
    {
        std::vector<std::pair<std::string, Compiler::TokenLoc>> mErrors;

        void report(const std::string& message, const Compiler::TokenLoc& loc, Compiler::ErrorHandler::Type type) override
        {
            if(type == Compiler::ErrorHandler::ErrorMessage)
                mErrors.emplace_back(message, loc);
        }

        void report(const std::string& message, Compiler::ErrorHandler::Type type) override
        {
            report(message, {}, type);
        }

    public:
        void reset() override
        {
            Compiler::ErrorHandler::reset();
            mErrors.clear();
        }

        const std::vector<std::pair<std::string, Compiler::TokenLoc>>& getErrors() const { return mErrors; }
    };

    struct CompiledScript
    {
        std::vector<Interpreter::Type_Code> mByteCode;
        Compiler::Locals mLocals;

        CompiledScript(const std::vector<Interpreter::Type_Code>& code, const Compiler::Locals& locals) : mByteCode(code), mLocals(locals) {}
    };

    struct MWScriptTest : public ::testing::Test
    {
        MWScriptTest() : mErrorHandler(), mParser(mErrorHandler, mCompilerContext) {}

        std::optional<CompiledScript> compile(const std::string& scriptBody)
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

        void run(const CompiledScript& script)
        {
            // mInterpreter.run(&script.mByteCode[0], script.mByteCode.size(), interpreterContext);
        }
    protected:
        void SetUp() override {}

        void TearDown() override {}
    private:
        TestErrorHandler mErrorHandler;
        TestCompilerContext mCompilerContext;
        Compiler::FileParser mParser;
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

    TEST_F(MWScriptTest, mwscript_test_invalid)
    {
        EXPECT_THROW(compile("this is not a valid script"), Compiler::SourceException);
    }

    TEST_F(MWScriptTest, mwscript_test_compilation)
    {
        auto script = compile(sScript1);
        logErrors();
        EXPECT_FALSE(!script);
    }
}