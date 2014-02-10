
#include "globalscripts.hpp"

#include <cassert>

#include <components/misc/stringops.hpp>
#include <components/esm/globalscript.hpp>

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

    int GlobalScripts::countSavedGameRecords() const
    {
        return mScripts.size();
    }

    void GlobalScripts::write (ESM::ESMWriter& writer) const
    {
        for (std::map<std::string, std::pair<bool, Locals> >::const_iterator iter (mScripts.begin());
            iter!=mScripts.end(); ++iter)
        {
            ESM::GlobalScript script;

            script.mId = iter->first;

            iter->second.second.write (script.mLocals, iter->first);

            script.mRunning = iter->second.first ? 1 : 0;

            writer.startRecord (ESM::REC_GSCR);
            script.save (writer);
            writer.endRecord (ESM::REC_GSCR);
        }
    }

    bool GlobalScripts::readRecord (ESM::ESMReader& reader, int32_t type)
    {
        if (type==ESM::REC_GSCR)
        {
            ESM::GlobalScript script;
            script.load (reader);

            std::map<std::string, std::pair<bool, Locals> >::iterator iter =
                mScripts.find (script.mId);

            if (iter==mScripts.end())
            {
                if (const ESM::Script *scriptRecord = mStore.get<ESM::Script>().search (script.mId))
                {
                    std::pair<bool, Locals> data (false, Locals());

                    data.second.configure (*scriptRecord);

                    iter = mScripts.insert (std::make_pair (script.mId, data)).first;
                }
                else // script does not exist anymore
                    return true;
            }

            iter->second.first = script.mRunning!=0;
            iter->second.second.read (script.mLocals, script.mId);

            return true;
        }

        return false;
    }

    Locals& GlobalScripts::getLocals (const std::string& name)
    {
        std::string name2 = Misc::StringUtils::lowerCase (name);
        std::map<std::string, std::pair<bool, Locals> >::iterator iter =
            mScripts.find (name2);

        if (iter==mScripts.end())
        {
            if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
            {
                Locals locals;

                locals.configure (*script);

                iter = mScripts.insert (std::make_pair (name, std::make_pair (false, locals))).first;
            }
        }

        return iter->second.second;
    }
}
