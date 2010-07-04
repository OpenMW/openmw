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
            std::map<std::string, std::pair<bool, Locals> > mScripts; // running, local variables
            
        public:
        
            GlobalScripts (const ESMS::ESMStore& store, ScriptManager& scriptManager);
    
            void addScript (const std::string& name);
            
            void removeScript (const std::string& name);
            
            bool isRunning (const std::string& name) const;
            
            void run (MWWorld::Environment& environment);
            ///< run all active global scripts
    };
}

#endif

