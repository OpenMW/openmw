#include "globalscripts.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/globalscript.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/loadsscr.hpp>
#include <components/misc/strings/lower.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/worldmodel.hpp"

#include "interpretercontext.hpp"

namespace
{
    struct ScriptCreatingVisitor
    {
        ESM::GlobalScript operator()(const MWWorld::Ptr& ptr) const
        {
            ESM::GlobalScript script;
            script.mRunning = false;
            if (!ptr.isEmpty())
            {
                if (MWBase::Environment::get().getWorld()->getPlayerPtr() == ptr)
                    script.mTargetId = ptr.getCellRef().getRefId();
                else if (ptr.getCellRef().getRefNum().isSet())
                {
                    script.mTargetId = ptr.getCellRef().getRefId();
                    script.mTargetRef = ptr.getCellRef().getRefNum();
                }
            }
            return script;
        }

        ESM::GlobalScript operator()(const std::pair<ESM::RefNum, ESM::RefId>& pair) const
        {
            ESM::GlobalScript script;
            script.mTargetId = pair.second;
            script.mTargetRef = pair.first;
            script.mRunning = false;
            return script;
        }
    };

    struct PtrGettingVisitor
    {
        const MWWorld::Ptr* operator()(const MWWorld::Ptr& ptr) const { return &ptr; }

        const MWWorld::Ptr* operator()(const std::pair<ESM::RefNum, ESM::RefId>& pair) const { return nullptr; }
    };

    struct PtrResolvingVisitor
    {
        MWWorld::Ptr operator()(const MWWorld::Ptr& ptr) const { return ptr; }

        MWWorld::Ptr operator()(const std::pair<ESM::RefNum, ESM::RefId>& pair) const
        {
            if (pair.first.isSet())
                return MWBase::Environment::get().getWorldModel()->getPtr(pair.first);
            else if (pair.second.empty())
                return MWWorld::Ptr();
            return MWBase::Environment::get().getWorld()->searchPtr(pair.second, false);
        }
    };

    class MatchPtrVisitor
    {
        const MWWorld::Ptr& mPtr;

    public:
        MatchPtrVisitor(const MWWorld::Ptr& ptr)
            : mPtr(ptr)
        {
        }

        bool operator()(const MWWorld::Ptr& ptr) const { return ptr == mPtr; }

        bool operator()(const std::pair<ESM::RefNum, ESM::RefId>& pair) const { return false; }
    };

    struct IdGettingVisitor
    {
        ESM::RefId operator()(const MWWorld::Ptr& ptr) const
        {
            if (ptr.isEmpty())
                return ESM::RefId();
            return ptr.mRef->mRef.getRefId();
        }

        ESM::RefId operator()(const std::pair<ESM::RefNum, ESM::RefId>& pair) const { return pair.second; }
    };
}

namespace MWScript
{
    GlobalScriptDesc::GlobalScriptDesc()
        : mRunning(false)
    {
    }

    const MWWorld::Ptr* GlobalScriptDesc::getPtrIfPresent() const
    {
        return std::visit(PtrGettingVisitor(), mTarget);
    }

    MWWorld::Ptr GlobalScriptDesc::getPtr()
    {
        MWWorld::Ptr ptr = std::visit(PtrResolvingVisitor{}, mTarget);
        mTarget = ptr;
        return ptr;
    }

    ESM::RefId GlobalScriptDesc::getId() const
    {
        return std::visit(IdGettingVisitor{}, mTarget);
    }

    GlobalScripts::GlobalScripts(const MWWorld::ESMStore& store)
        : mStore(store)
    {
    }

    void GlobalScripts::addScript(const ESM::RefId& name, const MWWorld::Ptr& target)
    {
        const auto iter = mScripts.find(name);

        if (iter == mScripts.end())
        {
            if (const ESM::Script* script = mStore.get<ESM::Script>().search(name))
            {
                auto desc = std::make_shared<GlobalScriptDesc>();
                MWWorld::Ptr ptr = target;
                desc->mTarget = ptr;
                desc->mRunning = true;
                desc->mLocals.configure(*script);
                mScripts.insert(std::make_pair(name, desc));
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

    void GlobalScripts::removeScript(const ESM::RefId& name)
    {
        const auto iter = mScripts.find(name);

        if (iter != mScripts.end())
            iter->second->mRunning = false;
    }

    bool GlobalScripts::isRunning(const ESM::RefId& name) const
    {
        const auto iter = mScripts.find(name);

        if (iter == mScripts.end())
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
        std::vector<ESM::RefId> scripts;

        scripts.emplace_back(ESM::RefId::stringRefId("main"));

        for (MWWorld::Store<ESM::StartScript>::iterator iter = mStore.get<ESM::StartScript>().begin();
             iter != mStore.get<ESM::StartScript>().end(); ++iter)
        {
            scripts.push_back(iter->mId);
        }

        // add scripts
        for (auto iter(scripts.begin()); iter != scripts.end(); ++iter)
        {
            try
            {
                addScript(*iter);
            }
            catch (const std::exception& exception)
            {
                Log(Debug::Error) << "Failed to add start script " << *iter << " because an exception has "
                                  << "been thrown: " << exception.what();
            }
        }
    }

    int GlobalScripts::countSavedGameRecords() const
    {
        return mScripts.size();
    }

    void GlobalScripts::write(ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (const auto& [id, desc] : mScripts)
        {
            ESM::GlobalScript script = std::visit(ScriptCreatingVisitor{}, desc->mTarget);

            script.mId = id;

            desc->mLocals.write(script.mLocals, id);

            script.mRunning = desc->mRunning;

            writer.startRecord(ESM::REC_GSCR);
            script.save(writer);
            writer.endRecord(ESM::REC_GSCR);
        }
    }

    bool GlobalScripts::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_GSCR)
        {
            ESM::GlobalScript script;
            script.load(reader);

            auto iter = mScripts.find(script.mId);

            if (iter == mScripts.end())
            {
                if (const ESM::Script* scriptRecord = mStore.get<ESM::Script>().search(script.mId))
                {
                    try
                    {
                        auto desc = std::make_shared<GlobalScriptDesc>();
                        if (!script.mTargetId.empty() || script.mTargetRef.isSet())
                        {
                            desc->mTarget = std::make_pair(script.mTargetRef, script.mTargetId);
                        }
                        desc->mLocals.configure(*scriptRecord);

                        iter = mScripts.insert(std::make_pair(script.mId, desc)).first;
                    }
                    catch (const std::exception& exception)
                    {
                        Log(Debug::Error) << "Failed to add start script " << script.mId
                                          << " because an exception has been thrown: " << exception.what();

                        return true;
                    }
                }
                else // script does not exist anymore
                    return true;
            }

            iter->second->mRunning = script.mRunning;
            iter->second->mLocals.read(script.mLocals, script.mId);

            return true;
        }

        return false;
    }

    Locals& GlobalScripts::getLocals(const ESM::RefId& name)
    {
        auto iter = mScripts.find(name);

        if (iter == mScripts.end())
        {
            const ESM::Script* script = mStore.get<ESM::Script>().find(name);

            auto desc = std::make_shared<GlobalScriptDesc>();
            desc->mLocals.configure(*script);

            iter = mScripts.emplace(name, desc).first;
        }

        return iter->second->mLocals;
    }

    const GlobalScriptDesc* GlobalScripts::getScriptIfPresent(const ESM::RefId& name) const
    {
        auto iter = mScripts.find(name);
        if (iter == mScripts.end())
            return nullptr;
        return iter->second.get();
    }

    void GlobalScripts::updatePtrs(const MWWorld::Ptr& base, const MWWorld::Ptr& updated)
    {
        MatchPtrVisitor visitor(base);
        for (const auto& script : mScripts)
        {
            if (std::visit(visitor, script.second->mTarget))
                script.second->mTarget = updated;
        }
    }
}
