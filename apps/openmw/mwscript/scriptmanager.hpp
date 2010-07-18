#ifndef GAME_SCRIPT_SCRIPTMANAGER_H
#define GAME_SCRIPT_SCRIPTMANAGER_H

#include <map>
#include <vector>
#include <string>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/compiler/fileparser.hpp>

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
    class Interpreter;
}

namespace MWScript
{    
    class ScriptManager
    {
            Compiler::StreamErrorHandler mErrorHandler;
            const ESMS::ESMStore& mStore;
            bool mVerbose;
            Compiler::Context& mCompilerContext;
            Compiler::FileParser mParser;
            
            std::map<std::string, std::vector<Interpreter::Type_Code> > mScripts;
            
            bool compile (const std::string& name);
            
        public:
        
            ScriptManager (const ESMS::ESMStore& store, bool verbose,
                Compiler::Context& compilerContext);
            
            void run (const std::string& name, Interpreter::Context& interpreterContext);
    };
};

#endif

