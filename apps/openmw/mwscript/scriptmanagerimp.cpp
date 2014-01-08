
#include "scriptmanagerimp.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>

#include <components/esm/loadscpt.hpp>
#include "../mwworld/esmstore.hpp"

#include <components/compiler/scanner.hpp>
#include <components/compiler/context.hpp>
#include <components/compiler/exception.hpp>

#include "extensions.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager (const MWWorld::ESMStore& store, bool verbose,
        Compiler::Context& compilerContext)
    : mErrorHandler (std::cerr), mStore (store), mVerbose (verbose),
      mCompilerContext (compilerContext), mParser (mErrorHandler, mCompilerContext),
      mOpcodesInstalled (false), mGlobalScripts (store)
    {}

    bool ScriptManager::compile (const std::string& name)
    {
        mParser.reset();
        mErrorHandler.reset();

        bool Success = true;

        if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
        {
            if (mVerbose)
                std::cout << "compiling script: " << name << std::endl;

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
                std::cerr << "An exception has been thrown: " << error.what() << std::endl;
                Success = false;
            }

            if (!Success && mVerbose)
            {
                std::cerr
                    << "compiling failed: " << name << std::endl
                    << script->mScriptText
                    << std::endl << std::endl;
            }

            if (Success)
            {
                std::vector<Interpreter::Type_Code> code;
                mParser.getCode (code);
                mScripts.insert (std::make_pair (name, std::make_pair (code, mParser.getLocals())));

                // TODO sanity check on generated locals

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
                std::cerr << "Execution of script " << name << " failed:" << std::endl;
                std::cerr << e.what() << std::endl;

                iter->second.first.clear(); // don't execute again.
            }
    }

    std::pair<int, int> ScriptManager::compileAll()
    {
        int count = 0;
        int success = 0;

        const MWWorld::Store<ESM::Script>& scripts = mStore.get<ESM::Script>();
        MWWorld::Store<ESM::Script>::iterator it = scripts.begin();

        for (; it != scripts.end(); ++it, ++count)
            if (compile (it->mId))
                ++success;

        return std::make_pair (count, success);
    }

    Compiler::Locals& ScriptManager::getLocals (const std::string& name)
    {
        {
            ScriptCollection::iterator iter = mScripts.find (name);

            if (iter!=mScripts.end())
                return iter->second.second;
        }

        {
            std::map<std::string, Compiler::Locals>::iterator iter = mOtherLocals.find (name);

            if (iter!=mOtherLocals.end())
                return iter->second;
        }

        Compiler::Locals locals;

        if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
        {
            int index = 0;

            for (int i=0; i<script->mData.mNumShorts; ++i)
                locals.declare ('s', script->mVarNames[index++]);

            for (int i=0; i<script->mData.mNumLongs; ++i)
                locals.declare ('l', script->mVarNames[index++]);

            for (int i=0; i<script->mData.mNumFloats; ++i)
                locals.declare ('f', script->mVarNames[index++]);

            std::map<std::string, Compiler::Locals>::iterator iter =
                mOtherLocals.insert (std::make_pair (name, locals)).first;

            return iter->second;
        }

        throw std::logic_error ("script " + name + " does not exist");
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }

    int ScriptManager::getLocalIndex (const std::string& scriptId, const std::string& variable,
        char type)
    {
        const ESM::Script *script = mStore.get<ESM::Script>().find (scriptId);

        int offset = 0;
        int size = 0;

        switch (type)
        {
            case 's':

                offset = 0;
                size = script->mData.mNumShorts;
                break;

            case 'l':

                offset = script->mData.mNumShorts;
                size = script->mData.mNumLongs;
                break;

            case 'f':

                offset = script->mData.mNumShorts+script->mData.mNumLongs;
                size = script->mData.mNumFloats;
                break;

            default:

                throw std::runtime_error ("invalid variable type");
        }

        for (int i=0; i<size; ++i)
            if (script->mVarNames.at (i+offset)==variable)
                return i;

        throw std::runtime_error ("unable to access local variable " + variable + " of " + scriptId);
    }

    void ScriptManager::resetGlobalScripts()
    {
        mGlobalScripts.reset();
    }
}
