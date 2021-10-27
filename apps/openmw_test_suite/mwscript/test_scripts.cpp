#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <components/compiler/context.hpp>
#include <components/compiler/errorhandler.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/extensions.hpp>
#include <components/compiler/extensions0.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/opcodes.hpp>
#include <components/compiler/scanner.hpp>

#include <components/interpreter/context.hpp>
#include <components/interpreter/installopcodes.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>

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

    class LocalVariables
    {
        std::vector<int> mShorts;
        std::vector<int> mLongs;
        std::vector<float> mFloats;

        template<class T>
        T getLocal(int index, const std::vector<T>& vector) const
        {
            if(index < vector.size())
                return vector[index];
            return {};
        }

        template<class T>
        void setLocal(T value, int index, std::vector<T>& vector)
        {
            if(index >= vector.size())
                vector.resize(index + 1);
            vector[index] = value;
        }
    public:
        void clear()
        {
            mShorts.clear();
            mLongs.clear();
            mFloats.clear();
        }

        int getShort(int index) const { return getLocal(index, mShorts); };

        int getLong(int index) const { return getLocal(index, mLongs); };

        float getFloat(int index) const { return getLocal(index, mFloats); };

        void setShort(int index, int value) { setLocal(value, index, mShorts); };

        void setLong(int index, int value) { setLocal(value, index, mLongs); };

        void setFloat(int index, float value) { setLocal(value, index, mFloats); };
    };

    class GlobalVariables
    {
        std::map<std::string, int> mShorts;
        std::map<std::string, int> mLongs;
        std::map<std::string, float> mFloats;

        template<class T>
        T getGlobal(const std::string& name, const std::map<std::string, T>& map) const
        {
            auto it = map.find(name);
            if(it != map.end())
                return it->second;
            return {};
        }
    public:
        void clear()
        {
            mShorts.clear();
            mLongs.clear();
            mFloats.clear();
        }

        int getShort(const std::string& name) const { return getGlobal(name, mShorts); };

        int getLong(const std::string& name) const { return getGlobal(name, mLongs); };

        float getFloat(const std::string& name) const { return getGlobal(name, mFloats); };

        void setShort(const std::string& name, int value) { mShorts[name] = value; };

        void setLong(const std::string& name, int value) { mLongs[name] = value; };

        void setFloat(const std::string& name, float value) { mFloats[name] = value; };
    };

    class TestInterpreterContext : public Interpreter::Context
    {
        LocalVariables mLocals;
        std::map<std::string, GlobalVariables> mMembers;
    public:
        std::string getTarget() const override { return {}; };

        int getLocalShort(int index) const override { return mLocals.getShort(index); };

        int getLocalLong(int index) const override { return mLocals.getLong(index); };

        float getLocalFloat(int index) const override { return mLocals.getFloat(index); };

        void setLocalShort(int index, int value) override { mLocals.setShort(index, value); };

        void setLocalLong(int index, int value) override { mLocals.setLong(index, value); };

        void setLocalFloat(int index, float value) override { mLocals.setFloat(index, value); };

        void messageBox(const std::string& message, const std::vector<std::string>& buttons) override {};

        void report(const std::string& message) override { std::cout << message << "\n"; };

        int getGlobalShort(const std::string& name) const override { return {}; };

        int getGlobalLong(const std::string& name) const override { return {}; };

        float getGlobalFloat(const std::string& name) const override { return {}; };

        void setGlobalShort(const std::string& name, int value) override {};

        void setGlobalLong(const std::string& name, int value) override {};

        void setGlobalFloat(const std::string& name, float value) override {};

        std::vector<std::string> getGlobals() const override { return {}; };

        char getGlobalType(const std::string& name) const override { return ' '; };

        std::string getActionBinding(const std::string& action) const override { return {}; };

        std::string getActorName() const override { return {}; };

        std::string getNPCRace() const override { return {}; };

        std::string getNPCClass() const override { return {}; };

        std::string getNPCFaction() const override { return {}; };

        std::string getNPCRank() const override { return {}; };

        std::string getPCName() const override { return {}; };

        std::string getPCRace() const override { return {}; };

        std::string getPCClass() const override { return {}; };

        std::string getPCRank() const override { return {}; };

        std::string getPCNextRank() const override { return {}; };

        int getPCBounty() const override { return {}; };

        std::string getCurrentCellName() const override { return {}; };

        int getMemberShort(const std::string& id, const std::string& name, bool global) const override
        {
            auto it = mMembers.find(id);
            if(it != mMembers.end())
                return it->second.getShort(name);
            return {};
        };

        int getMemberLong(const std::string& id, const std::string& name, bool global) const override
        {
            auto it = mMembers.find(id);
            if(it != mMembers.end())
                return it->second.getLong(name);
            return {};
        };

        float getMemberFloat(const std::string& id, const std::string& name, bool global) const override
        {
            auto it = mMembers.find(id);
            if(it != mMembers.end())
                return it->second.getFloat(name);
            return {};
        };

        void setMemberShort(const std::string& id, const std::string& name, int value, bool global) override { mMembers[id].setShort(name, value); };

        void setMemberLong(const std::string& id, const std::string& name, int value, bool global) override { mMembers[id].setLong(name, value); };

        void setMemberFloat(const std::string& id, const std::string& name, float value, bool global) override { mMembers[id].setFloat(name, value); };
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
            mInterpreter.run(&script.mByteCode[0], script.mByteCode.size(), context);
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
}