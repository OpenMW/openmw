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
                mParser.getCode (code);
                mScripts.insert (std::make_pair (name, std::make_pair (code, mParser.getLocals())));

                return true;
            }
        }

        return false;
    }

    void ScriptManager::run (const std::string& name, Interpreter::Context& interpreterContext)
    {
        // compile script
        ScriptCollection::iterator iter = mScripts.find (name);

        if (iter==mScripts.end())
        {
            if (!compile (name))
            {
                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                mScripts.insert (std::make_pair (name, std::make_pair (empty, Compiler::Locals())));
                return;
            }

            iter = mScripts.find (name);
            assert (iter!=mScripts.end());
        }

        // execute script
        if (!iter->second.first.empty())
            try
            {
                if (!mOpcodesInstalled)
                {
                    installOpcodes (mInterpreter);
                    mOpcodesInstalled = true;
                }

                mInterpreter.run (&iter->second.first[0], iter->second.first.size(), interpreterContext);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "Execution of script " << name << " failed:";
                Log(Debug::Error) << e.what();

                iter->second.first.clear(); // don't execute again.
            }
    }

    std::pair<int, int> ScriptManager::compileAll()
    {
        int count = 0;
        int success = 0;

        const MWWorld::Store<ESM::Script>& scripts = mStore.get<ESM::Script>();

        for (MWWorld::Store<ESM::Script>::iterator iter = scripts.begin();
            iter != scripts.end(); ++iter)
            if (!std::binary_search (mScriptBlacklist.begin(), mScriptBlacklist.end(),
                Misc::StringUtils::lowerCase (iter->mId)))
            {
                ++count;

                if (compile (iter->mId))
                    ++success;
            }

        return std::make_pair (count, success);
    }

    const Compiler::Locals& ScriptManager::getLocals (const std::string& name)
    {
        std::string name2 = Misc::StringUtils::lowerCase (name);

        {
            ScriptCollection::iterator iter = mScripts.find (name2);

            if (iter!=mScripts.end())
                return iter->second.second;
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
                mOtherLocals.insert (std::make_pair (name2, locals)).first;

            return iter->second;
        }

        throw std::logic_error ("script " + name + " does not exist");
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }
}
