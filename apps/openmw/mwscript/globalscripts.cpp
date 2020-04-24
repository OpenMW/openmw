#include "globalscripts.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/globalscript.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    GlobalScriptDesc::GlobalScriptDesc() : mRunning (false) {}


    GlobalScripts::GlobalScripts (const MWWorld::ESMStore& store)
    : mStore (store)
    {}

    void GlobalScripts::addScript (const std::string& name, const std::string& targetId)
    {
        std::map<std::string, GlobalScriptDesc>::iterator iter =
            mScripts.find (::Misc::StringUtils::lowerCase (name));

        if (iter==mScripts.end())
        {
            if (const ESM::Script *script = mStore.get<ESM::Script>().search(name))
            {
                GlobalScriptDesc desc;
                desc.mRunning = true;
                desc.mLocals.configure (*script);
                desc.mId = targetId;

                mScripts.insert (std::make_pair (name, desc));
            }
            else
            {
                Log(Debug::Error) << "Failed to add global script " << name << ": script record not found";
            }
        }
        else if (!iter->second.mRunning)
        {
            iter->second.mRunning = true;
            iter->second.mId = targetId;
        }
    }

    void GlobalScripts::removeScript (const std::string& name)
    {
        std::map<std::string, GlobalScriptDesc>::iterator iter =
            mScripts.find (::Misc::StringUtils::lowerCase (name));

        if (iter!=mScripts.end())
            iter->second.mRunning = false;
    }

    bool GlobalScripts::isRunning (const std::string& name) const
    {
        std::map<std::string, GlobalScriptDesc>::const_iterator iter =
            mScripts.find (::Misc::StringUtils::lowerCase (name));

        if (iter==mScripts.end())
            return false;

        return iter->second.mRunning;
    }

    void GlobalScripts::run()
    {
        for (std::map<std::string, GlobalScriptDesc>::iterator iter (mScripts.begin());
            iter!=mScripts.end(); ++iter)
        {
            if (iter->second.mRunning)
            {
                MWWorld::Ptr ptr;

                MWScript::InterpreterContext interpreterContext (
                    &iter->second.mLocals, MWWorld::Ptr(), iter->second.mId);

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
        // make list of global scripts to be added
        std::vector<std::string> scripts;

        scripts.push_back ("main");

        for (MWWorld::Store<ESM::StartScript>::iterator iter =
            mStore.get<ESM::StartScript>().begin();
            iter != mStore.get<ESM::StartScript>().end(); ++iter)
        {
            scripts.push_back (iter->mId);
        }

        // add scripts
        for (std::vector<std::string>::const_iterator iter (scripts.begin());
            iter!=scripts.end(); ++iter)
        {
            try
            {
                addScript (*iter);
            }
            catch (const std::exception& exception)
            {
                Log(Debug::Error)
                    << "Failed to add start script " << *iter << " because an exception has "
                    << "been thrown: " << exception.what();
            }
        }
    }

    int GlobalScripts::countSavedGameRecords() const
    {
        return mScripts.size();
    }

    void GlobalScripts::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (std::map<std::string, GlobalScriptDesc>::const_iterator iter (mScripts.begin());
            iter!=mScripts.end(); ++iter)
        {
            ESM::GlobalScript script;

            script.mId = iter->first;

            iter->second.mLocals.write (script.mLocals, iter->first);

            script.mRunning = iter->second.mRunning ? 1 : 0;

            script.mTargetId = iter->second.mId;

            writer.startRecord (ESM::REC_GSCR);
            script.save (writer);
            writer.endRecord (ESM::REC_GSCR);
        }
    }

    bool GlobalScripts::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        if (type==ESM::REC_GSCR)
        {
            ESM::GlobalScript script;
            script.load (reader);

            std::map<std::string, GlobalScriptDesc>::iterator iter =
                mScripts.find (script.mId);

            if (iter==mScripts.end())
            {
                if (const ESM::Script *scriptRecord = mStore.get<ESM::Script>().search (script.mId))
                {
                    try
                    {
                        GlobalScriptDesc desc;
                        desc.mLocals.configure (*scriptRecord);

                        iter = mScripts.insert (std::make_pair (script.mId, desc)).first;
                    }
                    catch (const std::exception& exception)
                    {
                        Log(Debug::Error)
                            << "Failed to add start script " << script.mId
                            << " because an exception has been thrown: " << exception.what();

                        return true;
                    }
                }
                else // script does not exist anymore
                    return true;
            }

            iter->second.mRunning = script.mRunning!=0;
            iter->second.mLocals.read (script.mLocals, script.mId);
            iter->second.mId = script.mTargetId;

            return true;
        }

        return false;
    }

    Locals& GlobalScripts::getLocals (const std::string& name)
    {
        std::string name2 = ::Misc::StringUtils::lowerCase (name);
        std::map<std::string, GlobalScriptDesc>::iterator iter = mScripts.find (name2);

        if (iter==mScripts.end())
        {
            const ESM::Script *script = mStore.get<ESM::Script>().find (name);

            GlobalScriptDesc desc;
            desc.mLocals.configure (*script);

            iter = mScripts.insert (std::make_pair (name2, desc)).first;
        }

        return iter->second.mLocals;
    }
}
