#include "interpretercontext.hpp"

#include <cmath>
#include <sstream>

#include <components/compiler/locals.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/records.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "globalscripts.hpp"
#include "locals.hpp"

namespace MWScript
{
    const MWWorld::Ptr InterpreterContext::getReferenceImp(const ESM::RefId& id, bool activeOnly, bool doThrow) const
    {
        if (!id.empty())
        {
            return MWBase::Environment::get().getWorld()->getPtr(id, activeOnly);
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

    const Locals& InterpreterContext::getMemberLocals(bool global, ESM::RefId& id) const
    {
        if (global)
        {
            return MWBase::Environment::get().getScriptManager()->getGlobalScripts().getLocals(id);
        }
        else
        {
            const MWWorld::Ptr ptr = getReferenceImp(id, false);

            id = ptr.getClass().getScript(ptr);

            ptr.getRefData().setLocals(*MWBase::Environment::get().getESMStore()->get<ESM::Script>().find(id));

            return ptr.getRefData().getLocals();
        }
    }

    Locals& InterpreterContext::getMemberLocals(bool global, ESM::RefId& id)
    {
        if (global)
        {
            return MWBase::Environment::get().getScriptManager()->getGlobalScripts().getLocals(id);
        }
        else
        {
            const MWWorld::Ptr ptr = getReferenceImp(id, false);

            id = ptr.getClass().getScript(ptr);

            ptr.getRefData().setLocals(*MWBase::Environment::get().getESMStore()->get<ESM::Script>().find(id));

            return ptr.getRefData().getLocals();
        }
    }

    MissingImplicitRefError::MissingImplicitRefError()
        : std::runtime_error("no implicit reference")
    {
    }

    int InterpreterContext::findLocalVariableIndex(const ESM::RefId& scriptId, std::string_view name, char type) const
    {
        int index = MWBase::Environment::get().getScriptManager()->getLocals(scriptId).searchIndex(type, name);

        if (index != -1)
            return index;

        std::ostringstream stream;

        stream << "Failed to access ";

        switch (type)
        {
            case 's':
                stream << "short";
                break;
            case 'l':
                stream << "long";
                break;
            case 'f':
                stream << "float";
                break;
        }

        stream << " member variable " << name << " in script " << scriptId;

        throw std::runtime_error(stream.str().c_str());
    }

    InterpreterContext::InterpreterContext(MWScript::Locals* locals, const MWWorld::Ptr& reference)
        : mLocals(locals)
        , mReference(reference)
    {
    }

    InterpreterContext::InterpreterContext(std::shared_ptr<GlobalScriptDesc> globalScriptDesc)
        : mLocals(&(globalScriptDesc->mLocals))
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

    ESM::RefId InterpreterContext::getTarget() const
    {
        if (!mReference.isEmpty())
            return mReference.mRef->mRef.getRefId();
        else if (mGlobalScriptDesc)
            return mGlobalScriptDesc->getId();
        return ESM::RefId();
    }

    int InterpreterContext::getLocalShort(int index) const
    {
        if (!mLocals)
            throw std::runtime_error("local variables not available in this context");

        return mLocals->mShorts.at(index);
    }

    int InterpreterContext::getLocalLong(int index) const
    {
        if (!mLocals)
            throw std::runtime_error("local variables not available in this context");

        return mLocals->mLongs.at(index);
    }

    float InterpreterContext::getLocalFloat(int index) const
    {
        if (!mLocals)
            throw std::runtime_error("local variables not available in this context");

        return mLocals->mFloats.at(index);
    }

    void InterpreterContext::setLocalShort(int index, int value)
    {
        if (!mLocals)
            throw std::runtime_error("local variables not available in this context");

        mLocals->mShorts.at(index) = value;
    }

    void InterpreterContext::setLocalLong(int index, int value)
    {
        if (!mLocals)
            throw std::runtime_error("local variables not available in this context");

        mLocals->mLongs.at(index) = value;
    }

    void InterpreterContext::setLocalFloat(int index, float value)
    {
        if (!mLocals)
            throw std::runtime_error("local variables not available in this context");

        mLocals->mFloats.at(index) = value;
    }

    void InterpreterContext::messageBox(std::string_view message, const std::vector<std::string>& buttons)
    {
        if (buttons.empty())
            MWBase::Environment::get().getWindowManager()->messageBox(message);
        else
            MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttons);
    }

    void InterpreterContext::report(const std::string& message) {}

    int InterpreterContext::getGlobalShort(std::string_view name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalInt(name);
    }

    int InterpreterContext::getGlobalLong(std::string_view name) const
    {
        // a global long is internally a float.
        return MWBase::Environment::get().getWorld()->getGlobalInt(name);
    }

    float InterpreterContext::getGlobalFloat(std::string_view name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalFloat(name);
    }

    void InterpreterContext::setGlobalShort(std::string_view name, int value)
    {
        MWBase::Environment::get().getWorld()->setGlobalInt(name, value);
    }

    void InterpreterContext::setGlobalLong(std::string_view name, int value)
    {
        MWBase::Environment::get().getWorld()->setGlobalInt(name, value);
    }

    void InterpreterContext::setGlobalFloat(std::string_view name, float value)
    {
        MWBase::Environment::get().getWorld()->setGlobalFloat(name, value);
    }

    std::vector<std::string> InterpreterContext::getGlobals() const
    {
        const MWWorld::Store<ESM::Global>& globals = MWBase::Environment::get().getESMStore()->get<ESM::Global>();

        std::vector<std::string> ids;
        for (const auto& globalVariable : globals)
        {
            ids.emplace_back(globalVariable.mId.getRefIdString());
        }

        return ids;
    }

    char InterpreterContext::getGlobalType(std::string_view name) const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        return world->getGlobalVariableType(name);
    }

    std::string InterpreterContext::getActionBinding(std::string_view targetAction) const
    {
        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        const auto& actions = input->getActionKeySorting();
        for (const int action : actions)
        {
            std::string_view desc = input->getActionDescription(action);
            if (desc.empty())
                continue;

            if (desc == targetAction)
            {
                if (input->joystickLastUsed())
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
        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(npc->mRace);
        return race->mName;
    }

    std::string_view InterpreterContext::getNPCClass() const
    {
        const ESM::NPC* npc = getReferenceImp().get<ESM::NPC>()->mBase;
        const ESM::Class* npcClass = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);
        return npcClass->mName;
    }

    std::string_view InterpreterContext::getNPCFaction() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        const ESM::RefId& factionId = ptr.getClass().getPrimaryFaction(ptr);
        if (factionId.empty())
        {
            Log(Debug::Warning) << "getNPCFaction(): NPC " << ptr.getCellRef().getRefId() << " has no primary faction";
            return "%";
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::ESMStore& store = world->getStore();
        const ESM::Faction* faction = store.get<ESM::Faction>().find(factionId);
        return faction->mName;
    }

    std::string_view InterpreterContext::getNPCRank() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        const MWWorld::Class& ptrClass = ptr.getClass();
        const ESM::RefId& factionId = ptrClass.getPrimaryFaction(ptr);
        if (factionId.empty())
        {
            Log(Debug::Warning) << "getNPCRank(): NPC " << ptr.getCellRef().getRefId() << " has no primary faction";
            return "%";
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::ESMStore& store = world->getStore();
        const ESM::Faction* faction = store.get<ESM::Faction>().find(factionId);

        int rank = ptrClass.getPrimaryFactionRank(ptr);
        if (rank < 0 || rank > 9)
        {
            Log(Debug::Warning) << "getNPCRank(): NPC " << ptr.getCellRef().getRefId() << " has invalid rank " << rank
                                << " in faction " << factionId;
            return "%";
        }
        return faction->mRanks[rank];
    }

    std::string_view InterpreterContext::getPCName() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        return world->getPlayerPtr().get<ESM::NPC>()->mBase->mName;
    }

    std::string_view InterpreterContext::getPCRace() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const ESM::RefId& race = world->getPlayerPtr().get<ESM::NPC>()->mBase->mRace;
        return world->getStore().get<ESM::Race>().find(race)->mName;
    }

    std::string_view InterpreterContext::getPCClass() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const ESM::RefId& playerClass = world->getPlayerPtr().get<ESM::NPC>()->mBase->mClass;
        return world->getStore().get<ESM::Class>().find(playerClass)->mName;
    }

    std::string_view InterpreterContext::getPCRank() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        const ESM::RefId& factionId = ptr.getClass().getPrimaryFaction(ptr);
        if (factionId.empty())
        {
            Log(Debug::Warning) << "getPCRank(): NPC " << ptr.getCellRef().getRefId() << " has no primary faction";
            return "%";
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        const auto& ranks = player.getClass().getNpcStats(player).getFactionRanks();
        auto it = ranks.find(factionId);
        int rank = -1;
        if (it != ranks.end())
            rank = it->second;

        // If you are not in the faction, PcRank returns the first rank, for whatever reason.
        // This is used by the dialogue when joining the Thieves Guild in Balmora.
        if (rank == -1)
            rank = 0;

        const MWWorld::ESMStore& store = world->getStore();
        const ESM::Faction* faction = store.get<ESM::Faction>().find(factionId);

        if (rank < 0 || rank > 9) // there are only 10 ranks
            return {};

        return faction->mRanks[rank];
    }

    std::string_view InterpreterContext::getPCNextRank() const
    {
        const MWWorld::Ptr& ptr = getReferenceImp();
        const ESM::RefId& factionId = ptr.getClass().getPrimaryFaction(ptr);
        if (factionId.empty())
        {
            Log(Debug::Warning) << "getPCNextRank(): NPC " << ptr.getCellRef().getRefId() << " has no primary faction";
            return "%";
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        const auto& ranks = player.getClass().getNpcStats(player).getFactionRanks();
        auto it = ranks.find(factionId);
        int rank = -1;
        if (it != ranks.end())
            rank = it->second;

        ++rank; // Next rank

        // if we are already at max rank, there is no next rank
        if (rank > 9)
            rank = 9;

        const MWWorld::ESMStore& store = world->getStore();
        const ESM::Faction* faction = store.get<ESM::Faction>().find(factionId);

        if (rank < 0)
            return {};

        return faction->mRanks[rank];
    }

    int InterpreterContext::getPCBounty() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayerPtr();
        return player.getClass().getNpcStats(player).getBounty();
    }

    std::string_view InterpreterContext::getCurrentCellName() const
    {
        return MWBase::Environment::get().getWorld()->getCellName();
    }

    void InterpreterContext::executeActivation(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor)
    {
        // MWScripted activations don't go through Lua because 1-frame delay can brake mwscripts.
#if 0
        MWBase::Environment::get().getLuaManager()->objectActivated(ptr, actor);

        // TODO: Enable this branch after implementing one of the options:
        // 1) Pause this mwscript (or maybe all mwscripts) for one frame and continue from the same
        //     command when the activation is processed by Lua script.
        // 2) Force Lua scripts to handle a zero-length extra frame right now, so when control
        //     returns to the mwscript, the activation is already processed.
#else
        std::unique_ptr<MWWorld::Action> action = (ptr.getClass().activate(ptr, actor));
        action->execute(actor);
        if (action->getTarget() != MWWorld::Ptr() && action->getTarget() != ptr)
        {
            updatePtr(ptr, action->getTarget());
        }
#endif
    }

    int InterpreterContext::getMemberShort(ESM::RefId id, std::string_view name, bool global) const
    {
        const Locals& locals = getMemberLocals(global, id);

        return locals.mShorts[findLocalVariableIndex(id, name, 's')];
    }

    int InterpreterContext::getMemberLong(ESM::RefId id, std::string_view name, bool global) const
    {
        const Locals& locals = getMemberLocals(global, id);

        return locals.mLongs[findLocalVariableIndex(id, name, 'l')];
    }

    float InterpreterContext::getMemberFloat(ESM::RefId id, std::string_view name, bool global) const
    {
        const Locals& locals = getMemberLocals(global, id);

        return locals.mFloats[findLocalVariableIndex(id, name, 'f')];
    }

    void InterpreterContext::setMemberShort(ESM::RefId id, std::string_view name, int value, bool global)
    {
        Locals& locals = getMemberLocals(global, id);

        locals.mShorts[findLocalVariableIndex(id, name, 's')] = value;
    }

    void InterpreterContext::setMemberLong(ESM::RefId id, std::string_view name, int value, bool global)
    {
        Locals& locals = getMemberLocals(global, id);

        locals.mLongs[findLocalVariableIndex(id, name, 'l')] = value;
    }

    void InterpreterContext::setMemberFloat(ESM::RefId id, std::string_view name, float value, bool global)
    {
        Locals& locals = getMemberLocals(global, id);

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
