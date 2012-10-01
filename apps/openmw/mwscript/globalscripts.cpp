
#include "globalscripts.hpp"

#include <cassert>

#include <components/esm_store/reclists.hpp>
#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    GlobalScripts::GlobalScripts (const ESMS::ESMStore& store)
    : mStore (store)
    {
        addScript ("Main");

        for (ESMS::RecListT<ESM::StartScript>::MapType::const_iterator iter
            (store.startScripts.list.begin());
            iter != store.startScripts.list.end(); ++iter)
            addScript (iter->second.mScript);
    }

    void GlobalScripts::addScript (const std::string& name)
    {
        if (mScripts.find (name)==mScripts.end())
            if (const ESM::Script *script = mStore.scripts.find (name))
            {
                Locals locals;

                locals.configure (*script);

                mScripts.insert (std::make_pair (name, std::make_pair (true, locals)));
            }
    }

    void GlobalScripts::removeScript (const std::string& name)
    {
        std::map<std::string, std::pair<bool, Locals> >::iterator iter = mScripts.find (name);

        if (iter!=mScripts.end())
            iter->second.first = false;
    }

    bool GlobalScripts::isRunning (const std::string& name) const
    {
        std::map<std::string, std::pair<bool, Locals> >::const_iterator iter =
            mScripts.find (name);

        if (iter==mScripts.end())
            return false;

        return iter->second.first;
    }

    void GlobalScripts::run()
    {
        for (std::map<std::string, std::pair<bool, Locals> >::iterator iter (mScripts.begin());
            iter!=mScripts.end(); ++iter)
        {
            if (iter->second.first)
            {
                MWScript::InterpreterContext interpreterContext (
                    &iter->second.second, MWWorld::Ptr());
                MWBase::Environment::get().getScriptManager()->run (iter->first, interpreterContext);
            }
        }
    }
}
