#include "globalscripts.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/globalscript.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "interpretercontext.hpp"

namespace
{
    struct ScriptCreatingVisitor : public boost::static_visitor<ESM::GlobalScript>
    {
        ESM::GlobalScript operator()(const MWWorld::Ptr &ptr) const
        {
            ESM::GlobalScript script;
            script.mTargetRef.unset();
            script.mRunning = false;
            if (!ptr.isEmpty())
            {
                if (ptr.getCellRef().hasContentFile())
                {
                    script.mTargetId = ptr.getCellRef().getRefId();
                    script.mTargetRef = ptr.getCellRef().getRefNum();
                }
                else if (MWBase::Environment::get().getWorld()->getPlayerPtr() == ptr)
                    script.mTargetId = ptr.getCellRef().getRefId();
            }
            return script;
        }

        ESM::GlobalScript operator()(const std::pair<ESM::RefNum, std::string> &pair) const
        {
            ESM::GlobalScript script;
            script.mTargetId = pair.second;
            script.mTargetRef = pair.first;
            script.mRunning = false;
            return script;
        }
    };

    struct PtrGettingVisitor : public boost::static_visitor<const MWWorld::Ptr*>
    {
        const MWWorld::Ptr* operator()(const MWWorld::Ptr &ptr) const
        {
            return &ptr;
        }

        const MWWorld::Ptr* operator()(const std::pair<ESM::RefNum, std::string> &pair) const
        {
            return nullptr;
        }
    };

    struct PtrResolvingVisitor : public boost::static_visitor<MWWorld::Ptr>
    {
        MWWorld::Ptr operator()(const MWWorld::Ptr &ptr) const
        {
            return ptr;
        }

        MWWorld::Ptr operator()(const std::pair<ESM::RefNum, std::string> &pair) const
        {
            if (pair.second.empty())
                return MWWorld::Ptr();
            else if(pair.first.hasContentFile())
                return MWBase::Environment::get().getWorld()->searchPtrViaRefNum(pair.second, pair.first);
            return MWBase::Environment::get().getWorld()->searchPtr(pair.second, false);
        }
    };

    class MatchPtrVisitor : public boost::static_visitor<bool>
    {
        const MWWorld::Ptr& mPtr;
    public:
        MatchPtrVisitor(const MWWorld::Ptr& ptr) : mPtr(ptr) {}

        bool operator()(const MWWorld::Ptr &ptr) const
        {
            return ptr == mPtr;
        }

        bool operator()(const std::pair<ESM::RefNum, std::string> &pair) const
        {
            return false;
        }
    };

    struct IdGettingVisitor : public boost::static_visitor<std::string>
    {
        std::string operator()(const MWWorld::Ptr& ptr) const
        {
            if(ptr.isEmpty())
                return {};
            return ptr.mRef->mRef.getRefId();
        }

        std::string operator()(const std::pair<ESM::RefNum, std::string>& pair) const
        {
            return pair.second;
        }
    };
}

namespace MWScript
{
    GlobalScriptDesc::GlobalScriptDesc() : mRunning (false) {}

    const MWWorld::Ptr* GlobalScriptDesc::getPtrIfPresent() const
    {
        return boost::apply_visitor(PtrGettingVisitor(), mTarget);
    }

    MWWorld::Ptr GlobalScriptDesc::getPtr()
    {
        MWWorld::Ptr ptr = boost::apply_visitor(PtrResolvingVisitor(), mTarget);
        mTarget = ptr;
        return ptr;
    }

    std::string GlobalScriptDesc::getId() const
    {
        return boost::apply_visitor(IdGettingVisitor(), mTarget);
    }


    GlobalScripts::GlobalScripts (const MWWorld::ESMStore& store)
    : mStore (store)
    {}

    void GlobalScripts::addScript (const std::string& name, const MWWorld::Ptr& target)
    {
        const auto iter = mScripts.find (::Misc::StringUtils::lowerCase (name));

        if (iter==mScripts.end())
        {
            if (const ESM::Script *script = mStore.get<ESM::Script>().search(name))
            {
                auto desc = std::make_shared<GlobalScriptDesc>();
                MWWorld::Ptr ptr = target;
                desc->mTarget = ptr;
                desc->mRunning = true;
                desc->mLocals.configure (*script);
                mScripts.insert (std::make_pair(name, desc));
            }
            else
            {
                Log(Debug::Error) << "Failed to add global script " << name << ": script record not found";
            }
        }
        else if (!iter->second->mRunning)
        {
            iter->second->mRunning = true;
            MWWorld::Ptr ptr = target;
            iter->second->mTarget = ptr;
        }
    }

    void GlobalScripts::removeScript (const std::string& name)
    {
        const auto iter = mScripts.find (::Misc::StringUtils::lowerCase (name));

        if (iter!=mScripts.end())
            iter->second->mRunning = false;
    }

    bool GlobalScripts::isRunning (const std::string& name) const
    {
        const auto iter = mScripts.find (::Misc::StringUtils::lowerCase (name));

        if (iter==mScripts.end())
            return false;

        return iter->second->mRunning;
    }

    void GlobalScripts::run()
    {
        for (const auto& script : mScripts)
        {
            if (script.second->mRunning)
            {
                MWScript::InterpreterContext context(script.second);
                if (!MWBase::Environment::get().getScriptManager()->run(script.first, context))
                    script.second->mRunning = false;
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

        scripts.emplace_back("main");

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
        for (const auto& iter : mScripts)
        {
            ESM::GlobalScript script = boost::apply_visitor (ScriptCreatingVisitor(), iter.second->mTarget);

            script.mId = iter.first;

            iter.second->mLocals.write (script.mLocals, iter.first);

            script.mRunning = iter.second->mRunning ? 1 : 0;

            writer.startRecord (ESM::REC_GSCR);
            script.save (writer);
            writer.endRecord (ESM::REC_GSCR);
        }
    }

    bool GlobalScripts::readRecord (ESM::ESMReader& reader, uint32_t type, const std::map<int, int>& contentFileMap)
    {
        if (type==ESM::REC_GSCR)
        {
            ESM::GlobalScript script;
            script.load (reader);

            if (script.mTargetRef.hasContentFile())
            {
                auto iter = contentFileMap.find(script.mTargetRef.mContentFile);
                if (iter != contentFileMap.end())
                    script.mTargetRef.mContentFile = iter->second;
            }

            auto iter = mScripts.find (script.mId);

            if (iter==mScripts.end())
            {
                if (const ESM::Script *scriptRecord = mStore.get<ESM::Script>().search (script.mId))
                {
                    try
                    {
                        auto desc = std::make_shared<GlobalScriptDesc>();
                        if (!script.mTargetId.empty())
                        {
                            desc->mTarget = std::make_pair(script.mTargetRef, script.mTargetId);
                        }
                        desc->mLocals.configure (*scriptRecord);

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

            iter->second->mRunning = script.mRunning!=0;
            iter->second->mLocals.read (script.mLocals, script.mId);

            return true;
        }

        return false;
    }

    Locals& GlobalScripts::getLocals (const std::string& name)
    {
        std::string name2 = ::Misc::StringUtils::lowerCase (name);
        auto iter = mScripts.find (name2);

        if (iter==mScripts.end())
        {
            const ESM::Script *script = mStore.get<ESM::Script>().find (name);

            auto desc = std::make_shared<GlobalScriptDesc>();
            desc->mLocals.configure (*script);

            iter = mScripts.insert (std::make_pair (name2, desc)).first;
        }

        return iter->second->mLocals;
    }

    const Locals* GlobalScripts::getLocalsIfPresent (const std::string& name) const
    {
        std::string name2 = ::Misc::StringUtils::lowerCase (name);
        auto iter = mScripts.find (name2);
        if (iter==mScripts.end())
            return nullptr;
        return &iter->second->mLocals;
    }

    void GlobalScripts::updatePtrs(const MWWorld::Ptr& base, const MWWorld::Ptr& updated)
    {
        MatchPtrVisitor visitor(base);
        for (const auto& script : mScripts)
        {
            if (boost::apply_visitor (visitor, script.second->mTarget))
                script.second->mTarget = updated;
        }
    }
}
