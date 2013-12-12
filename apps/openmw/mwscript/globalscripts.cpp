
#include "globalscripts.hpp"

#include <cassert>

#include <components/misc/stringops.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    GlobalScripts::GlobalScripts (const MWWorld::ESMStore& store)
    : mStore (store)
    {
        addStartup();
    }

    void GlobalScripts::addScript (const std::string& name)
    {
        std::map<std::string, std::pair<bool, Locals> >::iterator iter =
            mScripts.find (Misc::StringUtils::lowerCase (name));

        if (iter==mScripts.end())
        {
            if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
            {
                Locals locals;

                locals.configure (*script);

                mScripts.insert (std::make_pair (name, std::make_pair (true, locals)));
            }
        }
        else
            iter->second.first = true;
    }

    void GlobalScripts::removeScript (const std::string& name)
    {
        std::map<std::string, std::pair<bool, Locals> >::iterator iter =
            mScripts.find (Misc::StringUtils::lowerCase (name));

        if (iter!=mScripts.end())
            iter->second.first = false;
    }

    bool GlobalScripts::isRunning (const std::string& name) const
    {
        std::map<std::string, std::pair<bool, Locals> >::const_iterator iter =
            mScripts.find (Misc::StringUtils::lowerCase (name));

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

    void GlobalScripts::clear()
    {
        mScripts.clear();
    }

    void GlobalScripts::addStartup()
    {
        addScript ("main");

        for (MWWorld::Store<ESM::StartScript>::iterator iter =
            mStore.get<ESM::StartScript>().begin();
            iter != mStore.get<ESM::StartScript>().end(); ++iter)
        {
            addScript (iter->mScript);
        }
    }
}
