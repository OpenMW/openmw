#include "scriptmanagerimp.hpp"

#include <algorithm>
#include <cassert>
#include <exception>
#include <sstream>

#include <components/debug/debuglog.hpp>

#include <components/esm/refid.hpp>
#include <components/esm3/loadscpt.hpp>

#include <components/misc/strings/lower.hpp>

#include <components/compiler/context.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/quickfileparser.hpp>
#include <components/compiler/scanner.hpp>

#include "../mwworld/esmstore.hpp"

#include "extensions.hpp"
#include "interpretercontext.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager(const MWWorld::ESMStore& store, Compiler::Context& compilerContext, int warningsMode)
        : mErrorHandler()
        , mStore(store)
        , mCompilerContext(compilerContext)
        , mParser(mErrorHandler, mCompilerContext)
        , mGlobalScripts(store)
    {
        installOpcodes(mInterpreter);

        mErrorHandler.setWarningsMode(warningsMode);
    }

    bool ScriptManager::compile(const ESM::RefId& name)
    {
        mParser.reset();
        mErrorHandler.reset();

        if (const ESM::Script* script = mStore.get<ESM::Script>().find(name))
        {
            mErrorHandler.setContext(script->mId.getRefIdString());

            bool success = true;
            try
            {
                std::istringstream input(script->mScriptText);

                Compiler::Scanner scanner(mErrorHandler, input, mCompilerContext.getExtensions());

                scanner.scan(mParser);

                if (!mErrorHandler.isGood())
                    success = false;
            }
            catch (const Compiler::SourceException&)
            {
                // error has already been reported via error handler
                success = false;
            }
            catch (const std::exception& error)
            {
                Log(Debug::Error) << "Error: An exception has been thrown: " << error.what();
                success = false;
            }

            if (!success)
            {
                Log(Debug::Error) << "Error: script compiling failed: " << name;
            }

            if (success)
            {
                mScripts.emplace(name, CompiledScript(mParser.getProgram(), mParser.getLocals()));

                return true;
            }
        }

        return false;
    }

    bool ScriptManager::run(const ESM::RefId& name, Interpreter::Context& interpreterContext)
    {
        // compile script
        auto iter = mScripts.find(name);

        if (iter == mScripts.end())
        {
            if (!compile(name))
            {
                // failed -> ignore script from now on.
                mScripts.emplace(name, CompiledScript({}, Compiler::Locals()));
                return false;
            }

            iter = mScripts.find(name);
            assert(iter != mScripts.end());
        }

        // execute script
        const auto& target = interpreterContext.getTarget();
        if (!iter->second.mProgram.mInstructions.empty()
            && iter->second.mInactive.find(target) == iter->second.mInactive.end())
        {
            try
            {
                mInterpreter.run(iter->second.mProgram, interpreterContext);
                return true;
            }
            catch (const MissingImplicitRefError& e)
            {
                Log(Debug::Error) << "Execution of script " << name << " failed: " << e.what();
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "Execution of script " << name << " failed: " << e.what();

                iter->second.mInactive.insert(target); // don't execute again.
            }
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
            ++count;

            if (compile(script.mId))
                ++success;
        }

        return std::make_pair(count, success);
    }

    const Compiler::Locals& ScriptManager::getLocals(const ESM::RefId& name)
    {
        {
            auto iter = mScripts.find(name);

            if (iter != mScripts.end())
                return iter->second.mLocals;
        }

        {
            auto iter = mOtherLocals.find(name);

            if (iter != mOtherLocals.end())
                return iter->second;
        }

        if (const ESM::Script* script = mStore.get<ESM::Script>().search(name))
        {
            Compiler::Locals locals;

            const Compiler::ContextOverride override(mErrorHandler, name.getRefIdString() + "[local variables]");

            std::istringstream stream(script->mScriptText);
            Compiler::QuickFileParser parser(mErrorHandler, mCompilerContext, locals);
            Compiler::Scanner scanner(mErrorHandler, stream, mCompilerContext.getExtensions());
            try
            {
                scanner.scan(parser);
            }
            catch (const Compiler::SourceException&)
            {
                // error has already been reported via error handler
                locals.clear();
            }
            catch (const std::exception& error)
            {
                Log(Debug::Error) << "Error: An exception has been thrown: " << error.what();
                locals.clear();
            }

            auto iter = mOtherLocals.emplace(name, locals).first;

            return iter->second;
        }

        throw std::logic_error("script " + name.toDebugString() + " does not exist");
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }

    const Compiler::Extensions& ScriptManager::getExtensions() const
    {
        return *mCompilerContext.getExtensions();
    }
}
