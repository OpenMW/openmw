#ifndef GAME_SCRIPT_SCRIPTMANAGER_H
#define GAME_SCRIPT_SCRIPTMANAGER_H

#include <map>
#include <set>
#include <string>

#include <components/compiler/fileparser.hpp>
#include <components/compiler/streamerrorhandler.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/types.hpp>

#include "../mwbase/scriptmanager.hpp"

#include "globalscripts.hpp"

namespace MWWorld
{
    class ESMStore;
}

namespace Compiler
{
    class Context;
}

namespace Interpreter
{
    class Context;
    class Interpreter;
}

namespace MWScript
{
    class ScriptManager : public MWBase::ScriptManager
    {
        Compiler::StreamErrorHandler mErrorHandler;
        const MWWorld::ESMStore& mStore;
        Compiler::Context& mCompilerContext;
        Compiler::FileParser mParser;
        Interpreter::Interpreter mInterpreter;
        bool mOpcodesInstalled;

        struct CompiledScript
        {
            std::vector<Interpreter::Type_Code> mByteCode;
            Compiler::Locals mLocals;
            std::set<std::string> mInactive;

            CompiledScript(const std::vector<Interpreter::Type_Code>& code, const Compiler::Locals& locals)
                : mByteCode(code)
                , mLocals(locals)
            {
            }
        };

        std::unordered_map<std::string, CompiledScript, ::Misc::StringUtils::CiHash, ::Misc::StringUtils::CiEqual>
            mScripts;
        GlobalScripts mGlobalScripts;
        std::unordered_map<std::string, Compiler::Locals, ::Misc::StringUtils::CiHash, ::Misc::StringUtils::CiEqual>
            mOtherLocals;
        std::vector<std::string> mScriptBlacklist;

    public:
        ScriptManager(const MWWorld::ESMStore& store, Compiler::Context& compilerContext, int warningsMode,
            const std::vector<std::string>& scriptBlacklist);

        void clear() override;

        bool run(std::string_view name, Interpreter::Context& interpreterContext) override;
        ///< Run the script with the given name (compile first, if not compiled yet)

        bool compile(std::string_view name) override;
        ///< Compile script with the given namen
        /// \return Success?

        std::pair<int, int> compileAll() override;
        ///< Compile all scripts
        /// \return count, success

        const Compiler::Locals& getLocals(std::string_view name) override;
        ///< Return locals for script \a name.

        GlobalScripts& getGlobalScripts() override;

        const Compiler::Extensions& getExtensions() const override;
    };
}

#endif
