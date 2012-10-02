
#include "scriptmanagerimp.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>

#include <components/esm/loadscpt.hpp>
#include <components/esm_store/store.hpp>

#include <components/compiler/scanner.hpp>
#include <components/compiler/context.hpp>
#include <components/compiler/exception.hpp>

#include "extensions.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager (const ESMS::ESMStore& store, bool verbose,
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

        if (const ESM::Script *script = mStore.scripts.find (name))
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
                std::cerr << "exeution of script " << name << " failed." << std::endl;

                if (mVerbose)
                    std::cerr << "(" << e.what() << ")" << std::endl;

                iter->second.first.clear(); // don't execute again.
            }
    }

    std::pair<int, int> ScriptManager::compileAll()
    {
        typedef ESMS::ScriptListT<ESM::Script>::MapType Container;

        const Container& scripts = mStore.scripts.list;

        int count = 0;
        int success = 0;

        for (Container::const_iterator iter (scripts.begin()); iter!=scripts.end(); ++iter, ++count)
            if (compile (iter->first))
                ++success;

        return std::make_pair (count, success);
    }

    Compiler::Locals& ScriptManager::getLocals (const std::string& name)
    {
        ScriptCollection::iterator iter = mScripts.find (name);

        if (iter==mScripts.end())
        {
            if (!compile (name))
            {
                /// \todo Handle case of cyclic member variable access. Currently this could look up
                /// the whole application in an endless recursion.

                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                mScripts.insert (std::make_pair (name, std::make_pair (empty, Compiler::Locals())));
                throw std::runtime_error ("failed to compile script " + name);
            }

            iter = mScripts.find (name);
        }

        return iter->second.second;
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }

    int ScriptManager::getLocalIndex (const std::string& scriptId, const std::string& variable,
        char type)
    {
        const ESM::Script *script = mStore.scripts.find (scriptId);

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

            default:

                throw std::runtime_error ("invalid variable type");
        }

        for (int i=0; i<size; ++i)
            if (script->mVarNames.at (i+offset)==variable)
                return i;

        throw std::runtime_error ("unable to access local variable " + variable + " of " + scriptId);
    }
}
