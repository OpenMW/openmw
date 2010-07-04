
#include "globalscripts.hpp"
#include "interpretercontext.hpp"
#include "scriptmanager.hpp"

namespace MWScript
{
    GlobalScripts::GlobalScripts (const ESMS::ESMStore& store, ScriptManager& scriptManager)
    : mStore (store), mScriptManager (scriptManager)
    {
        addScript ("Main");
    }

    void GlobalScripts::addScript (const std::string& name)
    {
        if (mScripts.find (name)==mScripts.end())
            if (const ESM::Script *script = mStore.scripts.find (name))
            {           
                Locals locals;
                
                locals.configure (*script);

                mScripts.insert (std::make_pair (name, locals));        
            }
    }
    
    void GlobalScripts::run (MWWorld::Environment& environment)
    {
        std::map<std::string, Locals>::iterator iter = mScripts.begin();
        
        while (iter!=mScripts.end())
        {
            MWScript::InterpreterContext interpreterContext (environment,
                &iter->second, MWWorld::Ptr());
            mScriptManager.run (iter->first, interpreterContext);        
        
            ++iter;
        }
    }
}

