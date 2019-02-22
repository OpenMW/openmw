#include "interpretercontext.hpp"

#include <cmath>
#include <stdexcept>
#include <sstream>

#include <components/compiler/locals.hpp>

#include <components/esm/cellid.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "locals.hpp"
#include "globalscripts.hpp"

namespace MWScript
{
    MWWorld::Ptr InterpreterContext::getReferenceImp (
        const std::string& id, bool activeOnly, bool doThrow)
    {
        if (!id.empty())
        {
            return MWBase::Environment::get().getWorld()->getPtr (id, activeOnly);
        }
        else
        {
            if (mReference.isEmpty() && !mTargetId.empty())
                mReference =
                    MWBase::Environment::get().getWorld()->searchPtr (mTargetId, false);

            if (mReference.isEmpty() && doThrow)
                throw std::runtime_error ("no implicit reference");

            return mReference;
        }
    }

    const MWWorld::Ptr InterpreterContext::getReferenceImp (
        const std::string& id, bool activeOnly, bool doThrow) const
    {
        if (!id.empty())
        {
            return MWBase::Environment::get().getWorld()->getPtr (id, activeOnly);
        }
        else
        {
            if (mReference.isEmpty() && !mTargetId.empty())
                mReference =
                    MWBase::Environment::get().getWorld()->searchPtr (mTargetId, false);

            if (mReference.isEmpty() && doThrow)
                throw std::runtime_error ("no implicit reference");

            return mReference;
        }
    }

    const Locals& InterpreterContext::getMemberLocals (std::string& id, bool global)
        const
    {
        if (global)
        {
            return MWBase::Environment::get().getScriptManager()->getGlobalScripts().
                getLocals (id);
        }
        else
        {
            const MWWorld::Ptr ptr = getReferenceImp (id, false);

             id = ptr.getClass().getScript (ptr);

            ptr.getRefData().setLocals (
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().find (id));

            return ptr.getRefData().getLocals();
        }
    }

    Locals& InterpreterContext::getMemberLocals (std::string& id, bool global)
    {
        if (global)
        {
            return MWBase::Environment::get().getScriptManager()->getGlobalScripts().
                getLocals (id);
        }
        else
        {
            const MWWorld::Ptr ptr = getReferenceImp (id, false);

            id = ptr.getClass().getScript (ptr);

            ptr.getRefData().setLocals (
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().find (id));

            return ptr.getRefData().getLocals();
        }
    }

    int InterpreterContext::findLocalVariableIndex (const std::string& scriptId,
        const std::string& name, char type) const
    {
        int index = MWBase::Environment::get().getScriptManager()->getLocals (scriptId).
            searchIndex (type, name);

        if (index!=-1)
            return index;

        std::ostringstream stream;

        stream << "Failed to access ";

        switch (type)
        {
            case 's': stream << "short"; break;
            case 'l': stream << "long"; break;
            case 'f': stream << "float"; break;
        }

        stream << " member variable " << name << " in script " << scriptId;

        throw std::runtime_error (stream.str().c_str());
    }


    InterpreterContext::InterpreterContext (
        MWScript::Locals *locals, const MWWorld::Ptr& reference, const std::string& targetId)
    : mLocals (locals), mReference (reference), mTargetId (targetId)
    {
        // If we run on a reference (local script, dialogue script or console with object
        // selected), store the ID of that reference store it so it can be inherited by
        // targeted scripts started from this one.
        if (targetId.empty() && !reference.isEmpty())
            mTargetId = reference.getCellRef().getRefId();
    }

    int InterpreterContext::getLocalShort (int index) const
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        return mLocals->mShorts.at (index);
    }

    int InterpreterContext::getLocalLong (int index) const
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        return mLocals->mLongs.at (index);
    }

    float InterpreterContext::getLocalFloat (int index) const
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        return mLocals->mFloats.at (index);
    }

    void InterpreterContext::setLocalShort (int index, int value)
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        mLocals->mShorts.at (index) = value;
    }

    void InterpreterContext::setLocalLong (int index, int value)
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        mLocals->mLongs.at (index) = value;
    }

    void InterpreterContext::setLocalFloat (int index, float value)
    {
        if (!mLocals)
            throw std::runtime_error ("local variables not available in this context");

        mLocals->mFloats.at (index) = value;
    }

    void InterpreterContext::messageBox (const std::string& message,
        const std::vector<std::string>& buttons)
    {
        if (buttons.empty())
            MWBase::Environment::get().getWindowManager()->messageBox (message);
        else
            MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttons);
    }

    void InterpreterContext::report (const std::string& message)
    {
    }

    bool InterpreterContext::menuMode()
    {
        return MWBase::Environment::get().getWindowManager()->isGuiMode();
    }

    int InterpreterContext::getGlobalShort (const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalInt (name);
    }

    int InterpreterContext::getGlobalLong (const std::string& name) const
    {
        // a global long is internally a float.
        return MWBase::Environment::get().getWorld()->getGlobalInt (name);
    }

    float InterpreterContext::getGlobalFloat (const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalFloat (name);
    }

    void InterpreterContext::setGlobalShort (const std::string& name, int value)
    {
        MWBase::Environment::get().getWorld()->setGlobalInt (name, value);
    }

    void InterpreterContext::setGlobalLong (const std::string& name, int value)
    {
        MWBase::Environment::get().getWorld()->setGlobalInt (name, value);
    }

    void InterpreterContext::setGlobalFloat (const std::string& name, float value)
    {
        MWBase::Environment::get().getWorld()->setGlobalFloat (name, value);
    }

    std::vector<std::string> InterpreterContext::getGlobals() const
    {
        std::vector<std::string> ids;

        const MWWorld::Store<ESM::Global>& globals =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Global>();

        for (MWWorld::Store<ESM::Global>::iterator iter = globals.begin(); iter!=globals.end();
            ++iter)
        {
            ids.push_back (iter->mId);
        }

        return ids;
    }

    char InterpreterContext::getGlobalType (const std::string& name) const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        return world->getGlobalVariableType(name);
    }

    std::string InterpreterContext::getActionBinding(const std::string& action) const
    {
        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        std::vector<int> actions = input->getActionKeySorting ();
        for (std::vector<int>::const_iterator it = actions.begin(); it != actions.end(); ++it)
        {
            std::string desc = input->getActionDescription (*it);
            if(desc == "")
                continue;

            if(desc == action)
            {
                if(input->joystickLastUsed())
                    return input->getActionControllerBindingName(*it);
                else
                    return input->getActionKeyBindingName (*it);
            }
        }

        return "None";
    }

    std::string InterpreterContext::getActorName() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        if (ptr.getClass().isNpc())
        {
            const ESM::NPC* npc = ptr.get<ESM::NPC>()->mBase;
            return npc->mName;
        }

        const ESM::Creature* creature = ptr.get<ESM::Creature>()->mBase;
        return creature->mName;
    }

    std::string InterpreterContext::getNPCRace() const
    {
        ESM::NPC npc = *getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npc.mRace);
        return race->mName;
    }

    std::string InterpreterContext::getNPCClass() const
    {
        ESM::NPC npc = *getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Class* class_ = MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(npc.mClass);
        return class_->mName;
    }

    std::string InterpreterContext::getNPCFaction() const
    {
        ESM::NPC npc = *getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Faction* faction = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(npc.mFaction);
        return faction->mName;
    }

    std::string InterpreterContext::getNPCRank() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        std::string faction = ptr.getClass().getPrimaryFaction(ptr);
        if (faction.empty())
            throw std::runtime_error("getNPCRank(): NPC is not in a faction");

        int rank = ptr.getClass().getPrimaryFactionRank(ptr);
        if (rank < 0 || rank > 9)
            throw std::runtime_error("getNPCRank(): invalid rank");

        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::ESMStore &store = world->getStore();
        const ESM::Faction *fact = store.get<ESM::Faction>().find(faction);
        return fact->mRanks[rank];
    }

    std::string InterpreterContext::getPCName() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        ESM::NPC player = *world->getPlayerPtr().get<ESM::NPC>()->mBase;
        return player.mName;
    }

    std::string InterpreterContext::getPCRace() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        std::string race = world->getPlayerPtr().get<ESM::NPC>()->mBase->mRace;
        return world->getStore().get<ESM::Race>().find(race)->mName;
    }

    std::string InterpreterContext::getPCClass() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        std::string class_ = world->getPlayerPtr().get<ESM::NPC>()->mBase->mClass;
        return world->getStore().get<ESM::Class>().find(class_)->mName;
    }

    std::string InterpreterContext::getPCRank() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();

        std::string factionId = getReferenceImp().getClass().getPrimaryFaction(getReferenceImp());
        if (factionId.empty())
            throw std::runtime_error("getPCRank(): NPC is not in a faction");

        const std::map<std::string, int>& ranks = player.getClass().getNpcStats (player).getFactionRanks();
        std::map<std::string, int>::const_iterator it = ranks.find(Misc::StringUtils::lowerCase(factionId));
        int rank = -1;
        if (it != ranks.end())
            rank = it->second;

        // If you are not in the faction, PcRank returns the first rank, for whatever reason.
        // This is used by the dialogue when joining the Thieves Guild in Balmora.
        if (rank == -1)
            rank = 0;

        const MWWorld::ESMStore &store = world->getStore();
        const ESM::Faction *faction = store.get<ESM::Faction>().find(factionId);

        if(rank < 0 || rank > 9) // there are only 10 ranks
            return "";

        return faction->mRanks[rank];
    }

    std::string InterpreterContext::getPCNextRank() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();

        std::string factionId = getReferenceImp().getClass().getPrimaryFaction(getReferenceImp());
        if (factionId.empty())
            throw std::runtime_error("getPCNextRank(): NPC is not in a faction");

        const std::map<std::string, int>& ranks = player.getClass().getNpcStats (player).getFactionRanks();
        std::map<std::string, int>::const_iterator it = ranks.find(Misc::StringUtils::lowerCase(factionId));
        int rank = -1;
        if (it != ranks.end())
            rank = it->second;

        ++rank; // Next rank

        // if we are already at max rank, there is no next rank
        if (rank > 9)
            rank = 9;

        const MWWorld::ESMStore &store = world->getStore();
        const ESM::Faction *faction = store.get<ESM::Faction>().find(factionId);

        if(rank < 0)
            return "";

        return faction->mRanks[rank];
    }

    int InterpreterContext::getPCBounty() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        return player.getClass().getNpcStats (player).getBounty();
    }

    std::string InterpreterContext::getCurrentCellName() const
    {
        return  MWBase::Environment::get().getWorld()->getCellName();
    }

    bool InterpreterContext::isScriptRunning (const std::string& name) const
    {
        return MWBase::Environment::get().getScriptManager()->getGlobalScripts().isRunning (name);
    }

    void InterpreterContext::startScript (const std::string& name, const std::string& targetId)
    {
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().addScript (name, targetId);
    }

    void InterpreterContext::stopScript (const std::string& name)
    {
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().removeScript (name);
    }

    float InterpreterContext::getDistance (const std::string& name, const std::string& id) const
    {
        // NOTE: id may be empty, indicating an implicit reference

        MWWorld::Ptr ref2;

        if (id.empty())
            ref2 = getReferenceImp();
        else
            ref2 = MWBase::Environment::get().getWorld()->getPtr(id, false);

        if (ref2.getContainerStore()) // is the object contained?
        {
            MWWorld::Ptr container = MWBase::Environment::get().getWorld()->findContainer(ref2);

            if (!container.isEmpty())
                ref2 = container;
            else
                throw std::runtime_error("failed to find container ptr");
        }

        const MWWorld::Ptr ref = MWBase::Environment::get().getWorld()->getPtr(name, false);

        // If the objects are in different worldspaces, return a large value (just like vanilla)
        if (!ref.isInCell() || !ref2.isInCell() || ref.getCell()->getCell()->getCellId().mWorldspace != ref2.getCell()->getCell()->getCellId().mWorldspace)
            return std::numeric_limits<float>::max();

        double diff[3];

        const float* const pos1 = ref.getRefData().getPosition().pos;
        const float* const pos2 = ref2.getRefData().getPosition().pos;
        for (int i=0; i<3; ++i)
            diff[i] = pos1[i] - pos2[i];

        return static_cast<float>(std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]));
    }

    void InterpreterContext::executeActivation(MWWorld::Ptr ptr, MWWorld::Ptr actor)
    {
        std::shared_ptr<MWWorld::Action> action = (ptr.getClass().activate(ptr, actor));
        action->execute (actor);
        if (action->getTarget() != MWWorld::Ptr() && action->getTarget() != ptr)
        {
            updatePtr(ptr, action->getTarget());
        }
    }

    float InterpreterContext::getSecondsPassed() const
    {
        return MWBase::Environment::get().getFrameDuration();
    }

    bool InterpreterContext::isDisabled (const std::string& id) const
    {
        const MWWorld::Ptr ref = getReferenceImp (id, false);
        return !ref.getRefData().isEnabled();
    }

    void InterpreterContext::enable (const std::string& id)
    {
        MWWorld::Ptr ref = getReferenceImp (id, false);
        MWBase::Environment::get().getWorld()->enable (ref);
    }

    void InterpreterContext::disable (const std::string& id)
    {
        MWWorld::Ptr ref = getReferenceImp (id, false);
        MWBase::Environment::get().getWorld()->disable (ref);
    }

    int InterpreterContext::getMemberShort (const std::string& id, const std::string& name,
        bool global) const
    {
        std::string scriptId (id);

        const Locals& locals = getMemberLocals (scriptId, global);

        return locals.mShorts[findLocalVariableIndex (scriptId, name, 's')];
    }

    int InterpreterContext::getMemberLong (const std::string& id, const std::string& name,
        bool global) const
    {
        std::string scriptId (id);

        const Locals& locals = getMemberLocals (scriptId, global);

        return locals.mLongs[findLocalVariableIndex (scriptId, name, 'l')];
    }

    float InterpreterContext::getMemberFloat (const std::string& id, const std::string& name,
        bool global) const
    {
        std::string scriptId (id);

        const Locals& locals = getMemberLocals (scriptId, global);

        return locals.mFloats[findLocalVariableIndex (scriptId, name, 'f')];
    }

    void InterpreterContext::setMemberShort (const std::string& id, const std::string& name,
        int value, bool global)
    {
        std::string scriptId (id);

        Locals& locals = getMemberLocals (scriptId, global);

        locals.mShorts[findLocalVariableIndex (scriptId, name, 's')] = value;
    }

    void InterpreterContext::setMemberLong (const std::string& id, const std::string& name, int value, bool global)
    {
        std::string scriptId (id);

        Locals& locals = getMemberLocals (scriptId, global);

        locals.mLongs[findLocalVariableIndex (scriptId, name, 'l')] = value;
    }

    void InterpreterContext::setMemberFloat (const std::string& id, const std::string& name, float value, bool global)
    {
        std::string scriptId (id);

        Locals& locals = getMemberLocals (scriptId, global);

        locals.mFloats[findLocalVariableIndex (scriptId, name, 'f')] = value;
    }

    MWWorld::Ptr InterpreterContext::getReference(bool required)
    {
        return getReferenceImp ("", true, required);
    }

    std::string InterpreterContext::getTargetId() const
    {
        return mTargetId;
    }

    void InterpreterContext::updatePtr(const MWWorld::Ptr& base, const MWWorld::Ptr& updated)
    {
        if (!mReference.isEmpty() && base == mReference)
        {
            mReference = updated;
            if (mLocals == &base.getRefData().getLocals())
                mLocals = &mReference.getRefData().getLocals();
        }
    }
}
