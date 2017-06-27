//
// Created by koncord on 02.01.16.
//

#include <boost/algorithm/clamp.hpp>
#include <components/openmw-mp/Log.hpp>
#include <apps/openmw/mwmechanics/steering.hpp>

#include "../mwbase/environment.hpp"

#include "../mwgui/windowmanagerimp.hpp"

#include "../mwclass/npc.hpp"

#include "../mwinput/inputmanagerimp.hpp"

#include "../mwmechanics/actor.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwstate/statemanagerimp.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"

#include "DedicatedPlayer.hpp"
#include "Main.hpp"
#include "GUIController.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"


using namespace mwmp;
using namespace std;

DedicatedPlayer::DedicatedPlayer(RakNet::RakNetGUID guid) : BasePlayer(guid)
{
    reference = 0;
    attack.pressed = 0;
    creatureStats.mDead = false;
    movementFlags = 0;
}
DedicatedPlayer::~DedicatedPlayer()
{

}

void DedicatedPlayer::update(float dt)
{
    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);

    MWMechanics::DynamicStat<float> value;

    if (creatureStats.mDead)
    {
        value.readState(creatureStats.mDynamic[0]);
        ptrCreatureStats->setHealth(value);
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        value.readState(creatureStats.mDynamic[i]);
        ptrCreatureStats->setDynamic(i, value);
    }

    if (ptrCreatureStats->isDead())
        ptrCreatureStats->resurrect();

    ptrCreatureStats->setAttacked(false);

    ptrCreatureStats->getAiSequence().stopCombat();

    ptrCreatureStats->setAlarmed(false);
    ptrCreatureStats->setAiSetting(MWMechanics::CreatureStats::AI_Alarm, 0);
    ptrCreatureStats->setAiSetting(MWMechanics::CreatureStats::AI_Fight, 0);
    ptrCreatureStats->setAiSetting(MWMechanics::CreatureStats::AI_Flee, 0);
    ptrCreatureStats->setAiSetting(MWMechanics::CreatureStats::AI_Hello, 0);

    // Only move and set anim flags if the framerate isn't too low
    if (dt < 0.1)
    {
        move(dt);
        setAnimFlags();
    }
}

void DedicatedPlayer::move(float dt)
{
    if (state != 2) return;

    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const int maxInterpolationDistance = 40;

    // Apply interpolation only if the position hasn't changed too much from last time
    bool shouldInterpolate = abs(position.pos[0] - refPos.pos[0]) < maxInterpolationDistance && abs(position.pos[1] - refPos.pos[1]) < maxInterpolationDistance && abs(position.pos[2] - refPos.pos[2]) < maxInterpolationDistance;

    if (shouldInterpolate)
    {
        static const int timeMultiplier = 15;
        osg::Vec3f lerp = MechanicsHelper::getLinearInterpolation(refPos.asVec3(), position.asVec3(), dt * timeMultiplier);
        refPos.pos[0] = lerp.x();
        refPos.pos[1] = lerp.y();
        refPos.pos[2] = lerp.z();

        world->moveObject(ptr, refPos.pos[0], refPos.pos[1], refPos.pos[2]);
    }
    else
    {
        world->moveObject(ptr, position.pos[0], position.pos[1], position.pos[2]);
    }

    float oldZ = ptr.getRefData().getPosition().rot[2];
    world->rotateObject(ptr, position.rot[0], 0, oldZ);

    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = direction.pos[0];
    move->mPosition[1] = direction.pos[1];

    MWMechanics::zTurn(ptr, position.rot[2], osg::DegreesToRadians(1.0));
}

void DedicatedPlayer::setAnimFlags()
{
    using namespace MWMechanics;

    MWBase::World *world = MWBase::Environment::get().getWorld();

    // Until we figure out a better workaround for disabling player gravity,
    // simply cast Levitate over and over on a player that's supposed to be flying
    if (!isFlying)
    {
        ptr.getClass().getCreatureStats(ptr).getActiveSpells().purgeEffect(ESM::MagicEffect::Levitate);
    }
    else if (isFlying && !world->isFlying(ptr))
    {
        MWMechanics::CastSpell cast(ptr, ptr);
        cast.mHitPosition = ptr.getRefData().getPosition().asVec3();
        cast.mAlwaysSucceed = true;
        cast.cast("Levitate");
    }

    if (drawState == 0)
        ptr.getClass().getCreatureStats(ptr).setDrawState(DrawState_Nothing);
    else if (drawState == 1)
        ptr.getClass().getCreatureStats(ptr).setDrawState(DrawState_Weapon);
    else if (drawState == 2)
        ptr.getClass().getCreatureStats(ptr).setDrawState(DrawState_Spell);

    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Run, (movementFlags & CreatureStats::Flag_Run) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_Sneak, (movementFlags & CreatureStats::Flag_Sneak) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceJump, (movementFlags & CreatureStats::Flag_ForceJump) != 0);
    ptrCreatureStats->setMovementFlag(CreatureStats::Flag_ForceMoveJump, (movementFlags & CreatureStats::Flag_ForceMoveJump) != 0);
}

void DedicatedPlayer::setEquipment()
{
    MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);

        const string &dedicItem = equipedItems[slot].refId;
        std::string item = "";
        bool equal = false;
        if (it != invStore.end())
        {
            item = it->getCellRef().getRefId();
            if (!Misc::StringUtils::ciEqual(item, dedicItem)) // if other item equiped
            {
                MWWorld::ContainerStore &store = ptr.getClass().getContainerStore(ptr);
                store.remove(item, store.count(item), ptr);
            }
            else
                equal = true;
        }

        if (dedicItem.empty() || equal)
            continue;

        const int count = equipedItems[slot].count;
        ptr.getClass().getContainerStore(ptr).add(dedicItem, count, ptr);

        for (MWWorld::ContainerStoreIterator it2 = invStore.begin(); it2 != invStore.end(); ++it2)
        {
            if (::Misc::StringUtils::ciEqual(it2->getCellRef().getRefId(), dedicItem)) // equip item
            {
                std::shared_ptr<MWWorld::Action> action = it2->getClass().use(*it2);
                action->execute(ptr);
                break;
            }
        }
    }
}

void DedicatedPlayer::setCell()
{
    // Prevent cell update when player hasn't been instantiated yet
    if (state == 0)
        return;

    MWBase::World *world = MWBase::Environment::get().getWorld();

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Server says DedicatedPlayer %s moved to %s",
        npc.mName.c_str(), cell.getDescription().c_str());

    MWWorld::CellStore *cellStore = Main::get().getCellController()->getCellStore(cell);

    if (!cellStore)
    {
        LOG_APPEND(Log::LOG_INFO, "%s", "- Cell doesn't exist on this client");
        world->disable(getPtr());
        return;
    }
    else
        world->enable(getPtr());

    // Allow this player's reference to move across a cell now that a manual cell
    // update has been called
    setPtr(world->moveObject(ptr, cellStore, position.pos[0], position.pos[1], position.pos[2]));

    // Remove the marker entirely if this player has moved to an interior that is inactive for us
    if (!cell.isExterior() && !Main::get().getCellController()->isActiveWorldCell(cell))
        removeMarker();
    // Otherwise, update their marker so the player shows up in the right cell on the world map
    else
        updateMarker();

    // If this player is now in a cell that we are the local authority over, we should send them all
    // NPC data in that cell
    if (Main::get().getCellController()->hasLocalAuthority(cell))
        Main::get().getCellController()->getCell(cell)->updateLocal(true);
}

void DedicatedPlayer::updateMarker()
{
    if (!markerEnabled)
        return;

    GUIController *gui = Main::get().getGUIController();

    if (gui->mPlayerMarkers.contains(marker))
    {
        gui->mPlayerMarkers.deleteMarker(marker);
        marker = gui->createMarker(guid);
        gui->mPlayerMarkers.addMarker(marker);
    }
    else
        gui->mPlayerMarkers.addMarker(marker, true);
}

void DedicatedPlayer::removeMarker()
{
    if (!markerEnabled)
        return;

    markerEnabled = false;
    GUIController *gui = Main::get().getGUIController();

    if (gui->mPlayerMarkers.contains(marker))
        Main::get().getGUIController()->mPlayerMarkers.deleteMarker(marker);
}

void DedicatedPlayer::setMarkerState(bool state)
{
    if (state)
    {
        markerEnabled = true;
        updateMarker();
    }
    else
        removeMarker();
}

MWWorld::Ptr DedicatedPlayer::getPtr()
{
    return ptr;
}

MWWorld::Ptr DedicatedPlayer::getLiveCellPtr()
{
    return reference->getPtr();
}

MWWorld::ManualRef *DedicatedPlayer::getRef()
{
    return reference;
}

void DedicatedPlayer::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;
}
