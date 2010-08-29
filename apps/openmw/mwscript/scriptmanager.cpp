
#include "scriptmanager.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>

#include <components/esm/loadscpt.hpp>
#include <components/esm_store/store.hpp>

#include <components/compiler/scanner.hpp>
#include <components/compiler/context.hpp>

#include <components/interpreter/interpreter.hpp>

#include "extensions.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager (const ESMS::ESMStore& store, bool verbose,
        Compiler::Context& compilerContext)
    : mErrorHandler (std::cerr), mStore (store), mVerbose (verbose),
      mCompilerContext (compilerContext), mParser (mErrorHandler, mCompilerContext)
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
                std::istringstream input (script->scriptText);

                Compiler::Scanner scanner (mErrorHandler, input, mCompilerContext.getExtensions());

                scanner.scan (mParser);

                if (!mErrorHandler.isGood())
                    Success = false;
            }
            catch (...)
            {
                Success = false;
            }

            if (!Success && mVerbose)
            {
                std::cerr
                    << "compiling failed: " << name << std::endl
                    << script->scriptText
                    << std::endl << std::endl;
            }

            if (Success)
            {
                std::vector<Interpreter::Type_Code> code;
                mParser.getCode (code);
                mScripts.insert (std::make_pair (name, code));

                // TODO sanity check on generated locals

                return true;
            }
        }

        return false;
    }

    void ScriptManager::run (const std::string& name, Interpreter::Context& interpreterContext)
    {
        // compile script
        std::map<std::string, std::vector<Interpreter::Type_Code> >::iterator iter =
            mScripts.find (name);

        if (iter==mScripts.end())
        {
            if (!compile (name))
            {
                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                mScripts.insert (std::make_pair (name, empty));
                return;
            }

            iter = mScripts.find (name);
            assert (iter!=mScripts.end());
        }

        // execute script
        if (!iter->second.empty())
            try
            {
                Interpreter::Interpreter interpreter (interpreterContext);
                installOpcodes (interpreter);
                interpreter.run (&iter->second[0], iter->second.size());
            }
            catch (const std::exception& e)
            {
                std::cerr << "exeution of script " << name << " failed." << std::endl;

                if (mVerbose)
                    std::cerr << "(" << e.what() << ")" << std::endl;

                iter->second.clear(); // don't execute again.
            }
    }
}
