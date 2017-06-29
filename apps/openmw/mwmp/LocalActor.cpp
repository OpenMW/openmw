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
    position = ptr.getRefData().getPosition();

    mwmp::Main::get().getNetworking()->getActorList()->addCellChangeActor(*this);
}

void LocalActor::updatePosition(bool forceUpdate)
{
    bool posIsChanging = (direction.pos[0] != 0 || direction.pos[1] != 0 || direction.pos[2] != 0 ||
        direction.rot[0] != 0 || direction.rot[1] != 0 || direction.rot[2] != 0);

    if (forceUpdate || posIsChanging || posWasChanged)
    {
        posWasChanged = posIsChanging;
        position = ptr.getRefData().getPosition();
        mwmp::Main::get().getNetworking()->getActorList()->addPositionActor(*this);
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

    // Update stats when they become 0 or they have changed enough
    //
    // Also check for an oldHealth of 0 changing to something else for resurrected NPCs

    auto needUpdate = [](MWMechanics::DynamicStat<float> &oldVal, MWMechanics::DynamicStat<float> &newVal, int limit) {
        return oldVal != newVal && (newVal.getCurrent() == 0 || oldVal.getCurrent() == 0
                                     || abs(oldVal.getCurrent() - newVal.getCurrent()) > limit);
    };

    if (forceUpdate || needUpdate(oldHealth, health, 5) || needUpdate(oldMagicka, magicka, 10) ||
        needUpdate(oldFatigue, fatigue, 10))
    {
        oldHealth = health;
        oldMagicka = magicka;
        oldFatigue = fatigue;

        health.writeState(creatureStats.mDynamic[0]);
        magicka.writeState(creatureStats.mDynamic[1]);
        fatigue.writeState(creatureStats.mDynamic[2]);

        creatureStats.mDead = ptrCreatureStats->isDead();

        mwmp::Main::get().getNetworking()->getActorList()->addStatsDynamicActor(*this);
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
        auto &item = equipedItems[slot];

        if (it != invStore.end())
        {
            auto &cellRef = it->getCellRef();
            if (!::Misc::StringUtils::ciEqual(cellRef.getRefId(), item.refId))
            {
                equipmentChanged = true;

                item.refId = cellRef.getRefId();
                item.charge = cellRef.getCharge();
                if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                {
                    MWMechanics::WeaponType weaptype;
                    auto &_class = ptr.getClass();
                    MWMechanics::getActiveWeapon(_class.getCreatureStats(ptr), _class.getInventoryStore(ptr), &weaptype);
                    if (weaptype != MWMechanics::WeapType_Thrown)
                        item.count = 1;
                }
                else
                    item.count = invStore.count(cellRef.getRefId());
            }
        }
        else if (!item.refId.empty())
        {
            equipmentChanged = true;
            item.refId = "";
            item.count = 0;
            item.charge = 0;
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
            attack.success = MechanicsHelper::getSpellSuccess(attack.spellId, ptr);
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
