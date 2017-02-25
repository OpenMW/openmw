//
// Created by koncord on 02.01.16.
//

#include "DedicatedPlayer.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwbase/environment.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include "../mwinput/inputmanagerimp.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwclass/npc.hpp"
#include "../mwmechanics/actor.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/action.hpp"
#include "Main.hpp"
#include "GUIController.hpp"
#include "../mwworld/inventorystore.hpp"
#include <boost/algorithm/clamp.hpp>
#include <components/openmw-mp/Log.hpp>
using namespace mwmp;
using namespace std;

std::map<RakNet::RakNetGUID, DedicatedPlayer *> Players::players;

DedicatedPlayer::DedicatedPlayer(RakNet::RakNetGUID guid) : BasePlayer(guid)
{
    attack.pressed = 0;
    creatureStats.mDead = false;
    movementFlags = 0;
}
DedicatedPlayer::~DedicatedPlayer()
{

}

MWWorld::Ptr DedicatedPlayer::getPtr()
{
    return ptr;
}

void Players::createPlayer(RakNet::RakNetGUID guid)
{
    LOG_APPEND(Log::LOG_INFO, "%s", "- Setting up character info");

    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    ESM::NPC npc = *player.get<ESM::NPC>()->mBase;
    DedicatedPlayer *dedicPlayer = players[guid];

    npc.mRace = dedicPlayer->npc.mRace;
    npc.mHead = dedicPlayer->npc.mHead;
    npc.mHair = dedicPlayer->npc.mHair;
    npc.mClass = dedicPlayer->npc.mClass;
    npc.mName = dedicPlayer->npc.mName;
    npc.mFlags = dedicPlayer->npc.mFlags;

    if (dedicPlayer->state == 0)
    {
        npc.mId = "Dedicated Player";

        std::string recid = world->createRecord(npc)->mId;

        dedicPlayer->reference = new MWWorld::ManualRef(world->getStore(), recid, 1);
    }

    // Temporarily spawn or move player to exterior 0,0 whenever setting base info
    ESM::Position spawnPos;
    spawnPos.pos[0] = spawnPos.pos[1] = spawnPos.pos[2] = 0;
    MWWorld::CellStore *cellStore = world->getExterior(0, 0);

    if (dedicPlayer->state == 0)
    {
        LOG_APPEND(Log::LOG_INFO, "- Creating new reference pointer for %s",
            dedicPlayer->npc.mName.c_str());

        MWWorld::Ptr tmp = world->placeObject(dedicPlayer->reference->getPtr(), cellStore, spawnPos);

        dedicPlayer->ptr.mCell = tmp.mCell;
        dedicPlayer->ptr.mRef = tmp.mRef;

        dedicPlayer->cell = *dedicPlayer->ptr.getCell()->getCell();
        dedicPlayer->position = dedicPlayer->ptr.getRefData().getPosition();
    }
    else
    {
        LOG_APPEND(Log::LOG_INFO, "- Updating reference pointer for %s",
            dedicPlayer->npc.mName.c_str());

        dedicPlayer->ptr.getBase()->canChangeCell = true;
        dedicPlayer->updatePtr(world->moveObject(dedicPlayer->ptr, cellStore, spawnPos.pos[0], spawnPos.pos[1], spawnPos.pos[2]));

        npc.mId = players[guid]->ptr.get<ESM::NPC>()->mBase->mId;

        MWWorld::ESMStore *store = const_cast<MWWorld::ESMStore *>(&world->getStore());
        MWWorld::Store<ESM::NPC> *esm_store = const_cast<MWWorld::Store<ESM::NPC> *> (&store->get<ESM::NPC>());

        esm_store->insert(npc);

        dedicPlayer->updateCell();

        ESM::CustomMarker mEditingMarker = Main::get().getGUIController()->CreateMarker(guid);
        dedicPlayer->marker = mEditingMarker;
        dedicPlayer->setMarkerState(true);
    }

    dedicPlayer->guid = guid;
    dedicPlayer->state = 2;

    // Give this new character a fatigue of at least 1 so it doesn't spawn
    // on the ground
    dedicPlayer->creatureStats.mDynamic[2].mBase = 1;

    world->enable(players[guid]->ptr);
}


void Players::cleanUp()
{
    for (std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin(); it != players.end(); it++)
        delete it->second;
}

void Players::disconnectPlayer(RakNet::RakNetGUID guid)
{
    if (players[guid]->state > 1)
    {
        players[guid]->state = 1;

        // Remove player's marker
        players[guid]->setMarkerState(false);

        MWBase::World *world = MWBase::Environment::get().getWorld();
        world->disable(players[guid]->getPtr());

        // Move player to exterior 0,0
        ESM::Position newPos;
        newPos.pos[0] = newPos.pos[1] = newPos.pos[2] = 0;
        MWWorld::CellStore *cellStore = world->getExterior(0, 0);

        players[guid]->getPtr().getBase()->canChangeCell = true;
        world->moveObject(players[guid]->getPtr(), cellStore, newPos.pos[0], newPos.pos[1], newPos.pos[2]);
    }
}

DedicatedPlayer *Players::getPlayer(RakNet::RakNetGUID guid)
{
    return players[guid];
}

MWWorld::Ptr DedicatedPlayer::getLiveCellPtr()
{
    return reference->getPtr();
}

MWWorld::ManualRef *DedicatedPlayer::getRef()
{
    return reference;
}

ESM::Position Slerp(ESM::Position start, ESM::Position end, float percent)
{
    // dot product - the cosine of the angle between 2 vectors.
    float dot = start.asVec3() * end.asVec3();
    // clamp it to be in the range of Acos()
    // This may be unnecessary, but floating point
    // precision can be a fickle mistress.
    dot = boost::algorithm::clamp(dot, -1.0f, 1.0f);
    // acos(dot) returns the angle between start and end,
    // And multiplying that by percent returns the angle between
    // start and the final result.
    float theta = acos(dot) * percent;
    osg::Vec3f relativeVec = end.asVec3() - start.asVec3() * dot;
    relativeVec.normalize(); // Orthonormal basis

    // result
    osg::Vec3f tmp ((start.asVec3() * cos(theta)) + (relativeVec * sin(theta)));
    ESM::Position result;

    result.pos[0] = tmp.x();
    result.pos[1] = tmp.y();
    result.pos[2] = tmp.z();

    result.rot[0] = start.rot[0];
    result.rot[1] = start.rot[1];
    result.rot[2] = result.rot[2];
    return result;
}

osg::Vec3f Lerp(osg::Vec3f start, osg::Vec3f end, float percent)
{
    osg::Vec3f p(percent, percent, percent);

    return (start + osg::componentMultiply(p, (end - start)));
}

void DedicatedPlayer::move(float dt)
{
    if (state != 2) return;

    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();


    {
        osg::Vec3f lerp = Lerp(refPos.asVec3(), position.asVec3(), dt * 15);
        refPos.pos[0] = lerp.x();
        refPos.pos[1] = lerp.y();
        refPos.pos[2] = lerp.z();
        world->moveObject(ptr, refPos.pos[0], refPos.pos[1], refPos.pos[2]);
    }

    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = direction.pos[0];
    move->mPosition[1] = direction.pos[1];
    move->mPosition[2] = direction.pos[2];

    world->rotateObject(ptr, position.rot[0], position.rot[1], position.rot[2]);
}

void Players::update(float dt)
{
    for (std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin(); it != players.end(); it++)
    {
        DedicatedPlayer *pl = it->second;
        if (pl == 0) continue;

        MWMechanics::NpcStats *ptrNpcStats = &pl->ptr.getClass().getNpcStats(pl->getPtr());

        MWMechanics::DynamicStat<float> value;

        if (pl->creatureStats.mDead)
        {
            value.readState(pl->creatureStats.mDynamic[0]);
            ptrNpcStats->setHealth(value);
            continue;
        }

        value.readState(pl->creatureStats.mDynamic[0]);
        ptrNpcStats->setHealth(value);
        value.readState(pl->creatureStats.mDynamic[1]);
        ptrNpcStats->setMagicka(value);
        value.readState(pl->creatureStats.mDynamic[2]);
        ptrNpcStats->setFatigue(value);

        if (ptrNpcStats->isDead())
            ptrNpcStats->resurrect();


        ptrNpcStats->setAttacked(false);

        ptrNpcStats->getAiSequence().stopCombat();

        ptrNpcStats->setAlarmed(false);
        ptrNpcStats->setAiSetting(MWMechanics::CreatureStats::AI_Alarm, 0);
        ptrNpcStats->setAiSetting(MWMechanics::CreatureStats::AI_Fight, 0);
        ptrNpcStats->setAiSetting(MWMechanics::CreatureStats::AI_Flee, 0);
        ptrNpcStats->setAiSetting(MWMechanics::CreatureStats::AI_Hello, 0);

        ptrNpcStats->setBaseDisposition(255);
        pl->move(dt);
        pl->updateDrawState();
    }
}

void DedicatedPlayer::updatePtr(MWWorld::Ptr newPtr)
{
    ptr.mCell = newPtr.mCell;
    ptr.mRef = newPtr.mRef;
    ptr.mContainerStore = newPtr.mContainerStore;

    // Disallow this player's reference from moving across cells until
    // the correct packet is sent by the player
    ptr.getBase()->canChangeCell = false;
}


DedicatedPlayer *Players::newPlayer(RakNet::RakNetGUID guid)
{
    LOG_APPEND(Log::LOG_INFO, "- Creating new DedicatedPlayer with guid %lu",
        guid.g);

    players[guid] = new DedicatedPlayer(guid);
    players[guid]->state = 0;
    return players[guid];
}

void DedicatedPlayer::updateEquipment()
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
                boost::shared_ptr<MWWorld::Action> action = it2->getClass().use(*it2);
                action->execute(ptr);
                break;
            }
        }
    }
}

const std::string DedicatedPlayer::getAnim()
{
    static string anim;
    static string animDir;
    static string animWeap;

    MWMechanics::NpcStats *npcStats = &ptr.getClass().getNpcStats(ptr);

    if (movementFlags & MWMechanics::CreatureStats::Flag_Run)
        anim = "run";
    else if (movementFlags & MWMechanics::CreatureStats::Flag_Sneak)
        anim = "sneak";
    else
        anim = "walk";

    if (movementAnim != 0)
    {

        if (movementAnim == 3)
            animDir = "forward";
        else if (movementAnim == 4)
            animDir = "back";
        else if (movementAnim == 2)
            animDir = "left";
        else if (movementAnim == 1)
            animDir = "right";
    }
    else
    {
        anim = "idle";
        animDir = "";
    }

    if (npcStats->getDrawState() == MWMechanics::DrawState_Weapon)
    {
        MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponSlot = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

        if (weaponSlot != invStore.end() && weaponSlot->getTypeName() == typeid(ESM::Weapon).name())
        {
            int type = weaponSlot->get<ESM::Weapon>()->mBase->mData.mType;
            if (type == ESM::Weapon::ShortBladeOneHand ||
                type == ESM::Weapon::LongBladeOneHand ||
                type == ESM::Weapon::BluntOneHand ||
                type == ESM::Weapon::AxeOneHand /*||
                type == ESM::Weapon::MarksmanThrown ||
                type == ESM::Weapon::MarksmanCrossbow ||
                type == ESM::Weapon::MarksmanBow*/)
                animWeap = "1h";
            else if (type == ESM::Weapon::LongBladeTwoHand ||
                    type == ESM::Weapon::BluntTwoClose ||
                    type == ESM::Weapon::AxeTwoHand)
                animWeap = "2c";
            else if (type == ESM::Weapon::BluntTwoWide ||
                    type == ESM::Weapon::SpearTwoWide)
                animWeap = "2w";
        }
        else
            animWeap = "hh";
    }
    else if (movementAnim == 0 && npcStats->getDrawState() == MWMechanics::DrawState_Spell)
        animWeap = "spell";
    else
        animWeap = "";

    return (anim + animDir + animWeap);
}

DedicatedPlayer *Players::getPlayer(const MWWorld::Ptr &ptr)
{
    std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin();

    for (; it != players.end(); it++)
    {
        if (it->second == 0 || it->second->getPtr().mRef == 0)
            continue;
        string refid = ptr.getCellRef().getRefId();
        if (it->second->getPtr().getCellRef().getRefId() == refid)
            return it->second;
    }
    return 0;
}

void DedicatedPlayer::updateDrawState()
{

    using namespace MWMechanics;
    if (drawState == 0)
        ptr.getClass().getNpcStats(ptr).setDrawState(DrawState_Nothing);
    else if (drawState == 1)
        ptr.getClass().getNpcStats(ptr).setDrawState(DrawState_Weapon);
    else if (drawState == 2)
        ptr.getClass().getNpcStats(ptr).setDrawState(DrawState_Spell);

    MWMechanics::NpcStats *ptrNpcStats = &ptr.getClass().getNpcStats(ptr);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_Run, (movementFlags & CreatureStats::Flag_Run) != 0);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_Sneak, (movementFlags & CreatureStats::Flag_Sneak) != 0);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_ForceJump, (movementFlags & CreatureStats::Flag_ForceJump) != 0);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_ForceMoveJump, (movementFlags & CreatureStats::Flag_ForceMoveJump) != 0);
}

void DedicatedPlayer::updateCell()
{
    // Prevent cell update when player hasn't been instantiated yet
    if (state == 0)
        return;

    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::CellStore *cellStore;


    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Server says %s (%s) moved to %s",
        ptr.getBase()->mRef.getRefId().c_str(),
        this->npc.mName.c_str(),
        cell.getDescription().c_str());

    try
    {
        if (cell.isExterior())
            cellStore = world->getExterior(cell.mData.mX, cell.mData.mY);
        else
            cellStore = world->getInterior(cell.mName);
    }
    // If the intended cell doesn't exist on this client, use ToddTest as a replacement
    catch (std::exception&)
    {
        cellStore = world->getInterior("ToddTest");
        LOG_APPEND(Log::LOG_INFO, "%s", "- Cell doesn't exist on this client");
    }

    if (!cellStore) return;

    // Allow this player's reference to move across a cell now that a manual cell
    // update has been called
    ptr.getBase()->canChangeCell = true;
    updatePtr(world->moveObject(ptr, cellStore, position.pos[0], position.pos[1], position.pos[2]));
}


void DedicatedPlayer::updateMarker()
{
    if (!markerEnabled)
        return;

    GUIController *gui = Main::get().getGUIController();

    if (gui->mPlayerMarkers.contains(marker))
    {
        gui->mPlayerMarkers.deleteMarker(marker);
        marker = gui->CreateMarker(guid);
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

void DedicatedPlayer::updateActor(MWMechanics::Actor *actor)
{
    actor->getCharacterController()->setAttackingOrSpell(attack.pressed);
}
