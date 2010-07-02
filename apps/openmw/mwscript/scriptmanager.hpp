#ifndef GAME_SCRIPT_SCRIPTMANAGER_H
#define GAME_SCRIPT_SCRIPTMANAGER_H

#include <map>
#include <vector>
#include <string>

#include <components/interpreter/types.hpp>

namespace ESMS
{
    struct ESMStore;
}

namespace Compiler
{
    class Context;
}

namespace Interpreter
{
    class Context;
}

namespace MWScript
{
    struct Locals;
    
    class ScriptManager
    {
            const ESMS::ESMStore& mStore;
            bool mVerbose;
            
            std::map<std::string, std::vector<Interpreter::Type_Code> > mScripts;
            
        public:
        
            ScriptManager (const ESMS::ESMStore& store, bool verbose);
            
            void run (const std::string& name/*, Compiler::Context& compilerContext,
                Interpreter::Context& interpreterContext, Locals& locals*/);
    };
};

#endif


