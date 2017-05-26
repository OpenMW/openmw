#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"

#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalActor.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "ActorList.hpp"
#include "MechanicsHelper.hpp"

using namespace mwmp;
using namespace std;

LocalActor::LocalActor()
{
    posWasChanged = false;
    equipmentChanged = false;

    wasRunning = false;
    wasSneaking = false;
    wasForceJumping = false;
    wasForceMoveJumping = false;
    wasFlying = false;

    positionTimer = 0;
    statTimer = 0;

    attack.type = Attack::MELEE;
    attack.shouldSend = false;

    creatureStats.mDead = false;
}

LocalActor::~LocalActor()
{

}

void LocalActor::update(bool forceUpdate)
{
    updateStatsDynamic(forceUpdate);
    updateEquipment(forceUpdate);

    if (forceUpdate || !creatureStats.mDead)
    {
        updatePosition(forceUpdate);
        updateAnimFlags(forceUpdate);
        updateAnimPlay();
        updateSpeech();
        updateAttack();
    }
}

void LocalActor::updateCell()
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Sending ID_ACTOR_CELL_CHANGE about %s-%i-%i to server",
        refId.c_str(), refNumIndex, mpNum);

    LOG_APPEND(Log::LOG_INFO, "- Moved from %s to %s", cell.getDescription().c_str(), ptr.getCell()->getCell()->getDescription().c_str());

    cell = *ptr.getCell()->getCell();

    mwmp::Main::get().getNetworking()->getActorList()->addCellChangeActor(*this);
}

void LocalActor::updatePosition(bool forceUpdate)
{
    const float timeoutSec = 0.03;

    if (forceUpdate || (positionTimer += MWBase::Environment::get().getFrameDuration()) >= timeoutSec)
    {
        bool posIsChanging = (direction.pos[0] != 0 || direction.pos[1] != 0 || direction.pos[2] != 0 ||
            direction.rot[0] != 0 || direction.rot[1] != 0 || direction.rot[2] != 0);

        if (forceUpdate || posIsChanging || posWasChanged)
        {
            posWasChanged = posIsChanging;

            position = ptr.getRefData().getPosition();

            positionTimer = 0;

            mwmp::Main::get().getNetworking()->getActorList()->addPositionActor(*this);
        }
    }
}

void LocalActor::updateAnimFlags(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWMechanics::CreatureStats ptrCreatureStats = ptr.getClass().getCreatureStats(ptr);

    using namespace MWMechanics;

    bool isRunning = ptrCreatureStats.getMovementFlag(CreatureStats::Flag_Run);
    bool isSneaking = ptrCreatureStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool isForceJumping = ptrCreatureStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool isForceMoveJumping = ptrCreatureStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

    isFlying = world->isFlying(ptr);

    MWMechanics::DrawState_ currentDrawState = ptr.getClass().getCreatureStats(ptr).getDrawState();

    if (wasRunning != isRunning || wasSneaking != isSneaking ||
        wasForceJumping != isForceJumping || wasForceMoveJumping != isForceMoveJumping ||
        lastDrawState != currentDrawState || wasFlying != isFlying ||
        forceUpdate)
    {

        wasRunning = isRunning;
        wasSneaking = isSneaking;
        wasForceJumping = isForceJumping;
        wasForceMoveJumping = isForceMoveJumping;
        lastDrawState = currentDrawState;

        wasFlying = isFlying;

        movementFlags = 0;

#define __SETFLAG(flag, value) (value) ? (movementFlags | flag) : (movementFlags & ~flag)

        movementFlags = __SETFLAG(CreatureStats::Flag_Sneak, isSneaking);
        movementFlags = __SETFLAG(CreatureStats::Flag_Run, isRunning);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isForceJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceMoveJump, isForceMoveJumping);

#undef __SETFLAG

        drawState = currentDrawState;

        mwmp::Main::get().getNetworking()->getActorList()->addAnimFlagsActor(*this);
    }
}

void LocalActor::updateAnimPlay()
{
    if (!animation.groupname.empty())
    {
        mwmp::Main::get().getNetworking()->getActorList()->addAnimPlayActor(*this);
        animation.groupname.clear();
    }
}

void LocalActor::updateSpeech()
{
    if (!sound.empty())
    {
        mwmp::Main::get().getNetworking()->getActorList()->addSpeechActor(*this);
        sound.clear();
    }
}

void LocalActor::updateStatsDynamic(bool forceUpdate)
{
    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    MWMechanics::DynamicStat<float> health(ptrCreatureStats->getHealth());
    MWMechanics::DynamicStat<float> magicka(ptrCreatureStats->getMagicka());
    MWMechanics::DynamicStat<float> fatigue(ptrCreatureStats->getFatigue());

    const float timeoutSec = 0.5;

    if (forceUpdate || (statTimer += MWBase::Environment::get().getFrameDuration()) >= timeoutSec)
    {
        // Update stats when they become 0 or they have changed enough
        //
        // Also check for an oldHealth of 0 changing to something else for resurrected NPCs
        bool shouldUpdateHealth = oldHealth != health && (health.getCurrent() == 0 || oldHealth.getCurrent() == 0 || abs(oldHealth.getCurrent() - health.getCurrent()) > 5);
        bool shouldUpdateMagicka = false;
        bool shouldUpdateFatigue = false;

        if (!shouldUpdateHealth)
            shouldUpdateMagicka = oldMagicka != magicka && (magicka.getCurrent() == 0 || abs(oldMagicka.getCurrent() - magicka.getCurrent()) > 10);

        if (!shouldUpdateMagicka)
            shouldUpdateFatigue = oldFatigue != fatigue && (fatigue.getCurrent() == 0 || abs(oldFatigue.getCurrent() - fatigue.getCurrent()) > 10);
        
        if (forceUpdate || shouldUpdateHealth || shouldUpdateMagicka || shouldUpdateFatigue)
        {
            oldHealth = health;
            oldMagicka = magicka;
            oldFatigue = fatigue;

            health.writeState(creatureStats.mDynamic[0]);
            magicka.writeState(creatureStats.mDynamic[1]);
            fatigue.writeState(creatureStats.mDynamic[2]);

            creatureStats.mDead = ptrCreatureStats->isDead();

            statTimer = 0;

            mwmp::Main::get().getNetworking()->getActorList()->addStatsDynamicActor(*this);
        }
    }
}

void LocalActor::updateEquipment(bool forceUpdate)
{
    if (!ptr.getClass().hasInventoryStore(ptr))
        return;

    if (forceUpdate)
        equipmentChanged = true;

    MWWorld::InventoryStore &invStore = ptr.getClass().getInventoryStore(ptr);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; slot++)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
        if (it != invStore.end() && !::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), equipedItems[slot].refId))
        {
            equipmentChanged = true;

            equipedItems[slot].refId = it->getCellRef().getRefId();
            equipedItems[slot].charge = it->getCellRef().getCharge();
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
            {
                MWMechanics::WeaponType weaptype;
                MWMechanics::getActiveWeapon(ptr.getClass().getCreatureStats(ptr), ptr.getClass().getInventoryStore(ptr), &weaptype);
                if (weaptype != MWMechanics::WeapType_Thrown)
                    equipedItems[slot].count = 1;
            }
            else
                equipedItems[slot].count = invStore.count(it->getCellRef().getRefId());
        }
        else if (it == invStore.end() && !equipedItems[slot].refId.empty())
        {
            equipmentChanged = true;
            equipedItems[slot].refId = "";
            equipedItems[slot].count = 0;
            equipedItems[slot].charge = 0;
        }
    }

    if (equipmentChanged)
    {
        mwmp::Main::get().getNetworking()->getActorList()->addEquipmentActor(*this);
        equipmentChanged = false;
    }
}

void LocalActor::updateAttack()
{
    if (attack.shouldSend)
    {
        if (attack.type == Attack::MAGIC)
        {
            MWMechanics::CreatureStats &attackerStats = ptr.getClass().getCreatureStats(ptr);
            attack.spellId = attackerStats.getSpells().getSelectedSpell();
            attack.success = mwmp::Main::get().getMechanicsHelper()->getSpellSuccess(attack.spellId, ptr);
        }

        mwmp::Main::get().getNetworking()->getActorList()->addAttackActor(*this);
        attack.shouldSend = false;
    }
}

MWWorld::Ptr LocalActor::getPtr()
{
    return ptr;
}

void LocalActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;

    refId = ptr.getCellRef().getRefId();
    refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    mpNum = ptr.getCellRef().getMpNum();

    lastDrawState = ptr.getClass().getCreatureStats(ptr).getDrawState();
    oldHealth = ptr.getClass().getCreatureStats(ptr).getHealth();
    oldMagicka = ptr.getClass().getCreatureStats(ptr).getMagicka();
    oldFatigue = ptr.getClass().getCreatureStats(ptr).getFatigue();
}
