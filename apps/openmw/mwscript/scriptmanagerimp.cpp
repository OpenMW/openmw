#include "scriptmanagerimp.hpp"

#include <cassert>
#include <sstream>
#include <exception>
#include <algorithm>

#include <components/debug/debuglog.hpp>

#include <components/esm/loadscpt.hpp>

#include <components/misc/stringops.hpp>

#include <components/compiler/scanner.hpp>
#include <components/compiler/context.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/quickfileparser.hpp>

#include "../mwworld/esmstore.hpp"

#include "extensions.hpp"
#include "interpretercontext.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager (const MWWorld::ESMStore& store,
        Compiler::Context& compilerContext, int warningsMode,
        const std::vector<std::string>& scriptBlacklist)
    : mErrorHandler(), mStore (store),
      mCompilerContext (compilerContext), mParser (mErrorHandler, mCompilerContext),
      mOpcodesInstalled (false), mGlobalScripts (store)
    {
        mErrorHandler.setWarningsMode (warningsMode);

        mScriptBlacklist.resize (scriptBlacklist.size());

        std::transform (scriptBlacklist.begin(), scriptBlacklist.end(),
            mScriptBlacklist.begin(), Misc::StringUtils::lowerCase);
        std::sort (mScriptBlacklist.begin(), mScriptBlacklist.end());
    }

    bool ScriptManager::compile (const std::string& name)
    {
        mParser.reset();
        mErrorHandler.reset();

        if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
        {
            mErrorHandler.setContext(name);

            bool Success = true;
            try
            {
                std::istringstream input (script->mScriptText);

                Compiler::Scanner scanner (mErrorHandler, input, mCompilerContext.getExtensions());

                scanner.scan (mParser);

                if (!mErrorHandler.isGood())
                    Success = false;
            }
            catch (const Compiler::SourceException&)
            {
                // error has already been reported via error handler
                Success = false;
            }
            catch (const std::exception& error)
            {
                Log(Debug::Error) << "Error: An exception has been thrown: " << error.what();
                Success = false;
            }

            if (!Success)
            {
                Log(Debug::Error) << "Error: script compiling failed: " << name;
            }

            if (Success)
            {
                std::vector<Interpreter::Type_Code> code;
                mParser.getCode(code);
                mScripts.emplace(name, CompiledScript(code, mParser.getLocals()));

                return true;
            }
        }

        return false;
    }

    bool ScriptManager::run (const std::string& name, Interpreter::Context& interpreterContext)
    {
        // compile script
        ScriptCollection::iterator iter = mScripts.find (name);

        if (iter==mScripts.end())
        {
            if (!compile (name))
            {
                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                mScripts.emplace(name, CompiledScript(empty, Compiler::Locals()));
                return false;
            }

            iter = mScripts.find (name);
            assert (iter!=mScripts.end());
        }

        // execute script
        std::string target = Misc::StringUtils::lowerCase(interpreterContext.getTarget());
        if (!iter->second.mByteCode.empty() && iter->second.mInactive.find(target) == iter->second.mInactive.end())
            try
            {
                if (!mOpcodesInstalled)
                {
                    installOpcodes (mInterpreter);
                    mOpcodesInstalled = true;
                }

                mInterpreter.run (&iter->second.mByteCode[0], iter->second.mByteCode.size(), interpreterContext);
                return true;
            }
            catch (const MissingImplicitRefError& e)
            {
                Log(Debug::Error) << "Execution of script " << name << " failed: "  << e.what();
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "Execution of script " << name << " failed: "  << e.what();

                iter->second.mInactive.insert(target); // don't execute again.
            }
        return false;
    }

    void ScriptManager::clear()
    {
        for (auto& script : mScripts)
        {
            script.second.mInactive.clear();
        }

        mGlobalScripts.clear();
    }

    std::pair<int, int> ScriptManager::compileAll()
    {
        int count = 0;
        int success = 0;

        for (auto& script : mStore.get<ESM::Script>())
        {
            if (!std::binary_search (mScriptBlacklist.begin(), mScriptBlacklist.end(),
                Misc::StringUtils::lowerCase(script.mId)))
            {
                ++count;

                if (compile(script.mId))
                    ++success;
            }
        }

        return std::make_pair (count, success);
    }

    const Compiler::Locals& ScriptManager::getLocals (const std::string& name)
    {
        std::string name2 = Misc::StringUtils::lowerCase (name);

        {
            ScriptCollection::iterator iter = mScripts.find (name2);

            if (iter!=mScripts.end())
                return iter->second.mLocals;
        }

        {
            std::map<std::string, Compiler::Locals>::iterator iter = mOtherLocals.find (name2);

            if (iter!=mOtherLocals.end())
                return iter->second;
        }

        if (const ESM::Script *script = mStore.get<ESM::Script>().search (name2))
        {
            Compiler::Locals locals;

            const Compiler::ContextOverride override(mErrorHandler, name2 + "[local variables]");

            std::istringstream stream (script->mScriptText);
            Compiler::QuickFileParser parser (mErrorHandler, mCompilerContext, locals);
            Compiler::Scanner scanner (mErrorHandler, stream, mCompilerContext.getExtensions());
            scanner.scan (parser);

            std::map<std::string, Compiler::Locals>::iterator iter =
                mOtherLocals.emplace(name2, locals).first;

            return iter->second;
        }

        throw std::logic_error ("script " + name + " does not exist");
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }
}
