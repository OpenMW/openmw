#ifndef GAME_SCRIPT_GLOBALSCRIPTS_H
#define GAME_SCRIPT_GLOBALSCRIPTS_H

#include <string>
#include <map>

#include "locals.hpp"

namespace ESMS
{
    struct ESMStore;
}

namespace MWWorld
{
    class Environment;
}

namespace MWScript
{
    class ScriptManager;

    class GlobalScripts
    {
            const ESMS::ESMStore& mStore;
            ScriptManager& mScriptManager;
            std::map<std::string, Locals> mScripts;
            
        public:
        
            GlobalScripts (const ESMS::ESMStore& store, ScriptManager& scriptManager);
    
            void addScript (const std::string& name);
            
            void run (MWWorld::Environment& environment);
    };
}

#endif

