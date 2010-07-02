
#include "scriptmanager.hpp"

#include <iostream>

namespace MWScript
{
    ScriptManager::ScriptManager (const ESMS::ESMStore& store, bool verbose)
    : mStore (store), mVerbose (verbose)
    {}
    
    void ScriptManager::run (const std::string& name/*, Compiler::Context& compilerContext,
        Interpreter::Context& interpreterContext, Locals& locals*/)
    {
        std::cout << "script request: " << name << std::endl;
    
    }
}

