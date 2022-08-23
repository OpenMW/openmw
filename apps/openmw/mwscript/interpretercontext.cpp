#include "interpretercontext.hpp"

#include <cmath>
#include <sstream>

#include <components/compiler/locals.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "locals.hpp"
#include "globalscripts.hpp"

namespace MWScript
{
    const MWWorld::Ptr InterpreterContext::getReferenceImp(std::string_view id, bool activeOnly, bool doThrow) const
    {
        if (!id.empty())
        {
            return MWBase::Environment::get().getWorld()->getPtr (id, activeOnly);
        }
        else
        {
            if (mReference.isEmpty() && mGlobalScriptDesc)
                mReference = mGlobalScriptDesc->getPtr();

            if (mReference.isEmpty() && doThrow)
                throw MissingImplicitRefError();

            return mReference;
        }
    }

    const Locals& InterpreterContext::getMemberLocals(std::string_view& id, bool global)
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

    Locals& InterpreterContext::getMemberLocals(std::string_view& id, bool global)
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

    MissingImplicitRefError::MissingImplicitRefError() : std::runtime_error("no implicit reference") {}

    int InterpreterContext::findLocalVariableIndex(std::string_view scriptId, std::string_view name, char type) const
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

    InterpreterContext::InterpreterContext (MWScript::Locals *locals, const MWWorld::Ptr& reference)
    : mLocals (locals), mReference (reference)
    {}

    InterpreterContext::InterpreterContext (std::shared_ptr<GlobalScriptDesc> globalScriptDesc)
    : mLocals (&(globalScriptDesc->mLocals))
    {
        const MWWorld::Ptr* ptr = globalScriptDesc->getPtrIfPresent();
        // A nullptr here signifies that the script's target has not yet been resolved after loading the game.
        // Script targets are lazily resolved to MWWorld::Ptrs (which can, upon resolution, be empty)
        // because scripts started through dialogue often don't use their implicit target.
        if (ptr)
            mReference = *ptr;
        else
            mGlobalScriptDesc = globalScriptDesc;
    }

    std::string_view InterpreterContext::getTarget() const
    {
        if(!mReference.isEmpty())
            return mReference.mRef->mRef.getRefId();
        else if(mGlobalScriptDesc)
            return mGlobalScriptDesc->getId();
        return {};
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

    void InterpreterContext::messageBox(std::string_view message,
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

    int InterpreterContext::getGlobalShort(std::string_view name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalInt (name);
    }

    int InterpreterContext::getGlobalLong(std::string_view name) const
    {
        // a global long is internally a float.
        return MWBase::Environment::get().getWorld()->getGlobalInt (name);
    }

    float InterpreterContext::getGlobalFloat(std::string_view name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalFloat (name);
    }

    void InterpreterContext::setGlobalShort(std::string_view name, int value)
    {
        MWBase::Environment::get().getWorld()->setGlobalInt (name, value);
    }

    void InterpreterContext::setGlobalLong(std::string_view name, int value)
    {
        MWBase::Environment::get().getWorld()->setGlobalInt (name, value);
    }

    void InterpreterContext::setGlobalFloat(std::string_view name, float value)
    {
        MWBase::Environment::get().getWorld()->setGlobalFloat (name, value);
    }

    std::vector<std::string> InterpreterContext::getGlobals() const
    {
        const MWWorld::Store<ESM::Global>& globals =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Global>();

        std::vector<std::string> ids;
        for (const auto& globalVariable : globals)
        {
            ids.emplace_back(globalVariable.mId);
        }

        return ids;
    }

    char InterpreterContext::getGlobalType(std::string_view name) const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        return world->getGlobalVariableType(name);
    }

    std::string InterpreterContext::getActionBinding(std::string_view targetAction) const
    {
        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        const auto& actions = input->getActionKeySorting();
        for (const int action : actions)
        {
            std::string_view desc = input->getActionDescription(action);
            if(desc.empty())
                continue;

            if(desc == targetAction)
            {
                if(input->joystickLastUsed())
                    return input->getActionControllerBindingName(action);
                else
                    return input->getActionKeyBindingName(action);
            }
        }

        return "None";
    }

    std::string_view InterpreterContext::getActorName() const
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

    std::string_view InterpreterContext::getNPCRace() const
    {
        const ESM::NPC* npc = getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npc->mRace);
        return race->mName;
    }

    std::string_view InterpreterContext::getNPCClass() const
    {
        const ESM::NPC* npc = getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Class* class_ = MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(npc->mClass);
        return class_->mName;
    }

    std::string_view InterpreterContext::getNPCFaction() const
    {
        const ESM::NPC* npc = getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Faction* faction = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(npc->mFaction);
        return faction->mName;
    }

    std::string_view InterpreterContext::getNPCRank() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        std::string_view faction = ptr.getClass().getPrimaryFaction(ptr);
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

    std::string_view InterpreterContext::getPCName() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        return world->getPlayerPtr().get<ESM::NPC>()->mBase->mName;
    }

    std::string_view InterpreterContext::getPCRace() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const std::string& race = world->getPlayerPtr().get<ESM::NPC>()->mBase->mRace;
        return world->getStore().get<ESM::Race>().find(race)->mName;
    }

    std::string_view InterpreterContext::getPCClass() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const std::string& class_ = world->getPlayerPtr().get<ESM::NPC>()->mBase->mClass;
        return world->getStore().get<ESM::Class>().find(class_)->mName;
    }

    std::string_view InterpreterContext::getPCRank() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();

        std::string_view factionId = getReferenceImp().getClass().getPrimaryFaction(getReferenceImp());
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
            return {};

        return faction->mRanks[rank];
    }

    std::string_view InterpreterContext::getPCNextRank() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();

        std::string_view factionId = getReferenceImp().getClass().getPrimaryFaction(getReferenceImp());
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
            return {};

        return faction->mRanks[rank];
    }

    int InterpreterContext::getPCBounty() const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        return player.getClass().getNpcStats (player).getBounty();
    }

    std::string_view InterpreterContext::getCurrentCellName() const
    {
        return MWBase::Environment::get().getWorld()->getCellName();
    }

    void InterpreterContext::executeActivation(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor)
    {
        MWBase::Environment::get().getLuaManager()->objectActivated(ptr, actor);
        std::unique_ptr<MWWorld::Action> action = (ptr.getClass().activate(ptr, actor));
        action->execute (actor);
        if (action->getTarget() != MWWorld::Ptr() && action->getTarget() != ptr)
        {
            updatePtr(ptr, action->getTarget());
        }
    }

    int InterpreterContext::getMemberShort(std::string_view id, std::string_view name,
        bool global) const
    {
        const Locals& locals = getMemberLocals(id, global);

        return locals.mShorts[findLocalVariableIndex(id, name, 's')];
    }

    int InterpreterContext::getMemberLong(std::string_view id, std::string_view name,
        bool global) const
    {
        const Locals& locals = getMemberLocals(id, global);

        return locals.mLongs[findLocalVariableIndex(id, name, 'l')];
    }

    float InterpreterContext::getMemberFloat(std::string_view id, std::string_view name,
        bool global) const
    {
        const Locals& locals = getMemberLocals(id, global);

        return locals.mFloats[findLocalVariableIndex(id, name, 'f')];
    }

    void InterpreterContext::setMemberShort(std::string_view id, std::string_view name,
        int value, bool global)
    {
        Locals& locals = getMemberLocals(id, global);

        locals.mShorts[findLocalVariableIndex(id, name, 's')] = value;
    }

    void InterpreterContext::setMemberLong(std::string_view id, std::string_view name, int value, bool global)
    {
        Locals& locals = getMemberLocals(id, global);

        locals.mLongs[findLocalVariableIndex(id, name, 'l')] = value;
    }

    void InterpreterContext::setMemberFloat(std::string_view id, std::string_view name, float value, bool global)
    {
        Locals& locals = getMemberLocals(id, global);

        locals.mFloats[findLocalVariableIndex(id, name, 'f')] = value;
    }

    MWWorld::Ptr InterpreterContext::getReference(bool required) const
    {
        return getReferenceImp({}, true, required);
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
