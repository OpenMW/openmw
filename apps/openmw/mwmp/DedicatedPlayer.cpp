//
// Created by koncord on 02.01.16.
//

#include "DedicatedPlayer.hpp"
#include <apps/openmw/mwmechanics/aitravel.hpp>
#include "../mwbase/environment.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include "../mwinput/inputmanagerimp.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwclass/npc.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/action.hpp"
#include "Main.hpp"
#include <apps/openmw/mwworld/inventorystore.hpp>
#include <boost/algorithm/clamp.hpp>
#include <components/openmw-mp/Log.hpp>
using namespace mwmp;
using namespace std;

std::map<RakNet::RakNetGUID, DedicatedPlayer *> Players::players;

DedicatedPlayer::DedicatedPlayer(RakNet::RakNetGUID guid) : BasePlayer(guid)
{
    GetAttack()->pressed = 0;
    CreatureStats()->mDead = false;
    movementFlags = 0;
}
DedicatedPlayer::~DedicatedPlayer()
{

}

MWWorld::Ptr DedicatedPlayer::getPtr()
{
    return ptr;
}

void Players::CreatePlayer(RakNet::RakNetGUID id)
{
    LOG_APPEND(Log::LOG_INFO, "%s", "- Setting up character info");

    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    ESM::NPC npc = *player.get<ESM::NPC>()->mBase;
    DedicatedPlayer *dedicPlayer = players[id];

    npc.mRace = dedicPlayer->Npc()->mRace;
    npc.mHead = dedicPlayer->Npc()->mHead;
    npc.mHair = dedicPlayer->Npc()->mHair;
    npc.mClass = dedicPlayer->Npc()->mClass;
    npc.mName = dedicPlayer->Npc()->mName;
    npc.mFlags = dedicPlayer->Npc()->mFlags;

    if (dedicPlayer->state == 0)
    {
        npc.mId = "Dedicated Player";

        std::string recid = world->createRecord(npc)->mId;

        dedicPlayer->reference = new MWWorld::ManualRef(world->getStore(), recid, 1);
    }

    // Temporarily spawn or move player to ToddTest whenever setting base info
    ESM::Position newPos;
    world->findInteriorPosition("ToddTest", newPos);
    MWWorld::CellStore *cellStore = world->getInterior("ToddTest");

    if (dedicPlayer->state == 0)
    {
        LOG_APPEND(Log::LOG_INFO, "- Creating new reference pointer for %s",
            dedicPlayer->Npc()->mName.c_str());

        MWWorld::Ptr tmp = world->placeObject(dedicPlayer->reference->getPtr(), cellStore, newPos);

        dedicPlayer->ptr.mCell = tmp.mCell;
        dedicPlayer->ptr.mRef = tmp.mRef;

        dedicPlayer->cell = *dedicPlayer->ptr.getCell()->getCell();
        dedicPlayer->pos = dedicPlayer->ptr.getRefData().getPosition();
    }
    else
    {
        LOG_APPEND(Log::LOG_INFO, "- Updating reference pointer for %s",
            dedicPlayer->Npc()->mName.c_str());

        dedicPlayer->ptr.getBase()->canChangeCell = true;
        dedicPlayer->UpdatePtr(world->moveObject(dedicPlayer->ptr, cellStore, newPos.pos[0], newPos.pos[1], newPos.pos[2]));

        npc.mId = players[id]->ptr.get<ESM::NPC>()->mBase->mId;

        MWWorld::ESMStore *store = const_cast<MWWorld::ESMStore *>(&world->getStore());
        MWWorld::Store<ESM::NPC> *esm_store = const_cast<MWWorld::Store<ESM::NPC> *> (&store->get<ESM::NPC>());

        esm_store->insert(npc);

        dedicPlayer->updateCell();
    }

    dedicPlayer->guid = id;
    dedicPlayer->state = 2;

    world->enable(players[id]->ptr);

    ESM::CustomMarker mEditingMarker = Main::get().getGUIController()->CreateMarker(id);
    dedicPlayer->marker = mEditingMarker;
    dedicPlayer->markerEnabled = true;
}


void Players::CleanUp()
{
    for (std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin(); it != players.end(); it++)
        delete it->second;
}

void Players::DisconnectPlayer(RakNet::RakNetGUID id)
{
    if (players[id]->state > 1)
    {
        players[id]->state = 1;
        MWBase::World *world = MWBase::Environment::get().getWorld();
        world->disable(players[id]->getPtr());

        // Move player to ToddTest
        ESM::Position newPos;
        world->findInteriorPosition("ToddTest", newPos);
        MWWorld::CellStore *store = world->getInterior("ToddTest");

        players[id]->getPtr().getBase()->canChangeCell = true;
        world->moveObject(players[id]->getPtr(), store, newPos.pos[0], newPos.pos[1], newPos.pos[2]);
    }
}

DedicatedPlayer *Players::GetPlayer(RakNet::RakNetGUID id)
{
    return players[id];
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

void DedicatedPlayer::Move(float dt)
{
    if (state != 2) return;

    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();


    {
        osg::Vec3f lerp = Lerp(refPos.asVec3(), pos.asVec3(), dt * 15);
        refPos.pos[0] = lerp.x();
        refPos.pos[1] = lerp.y();
        refPos.pos[2] = lerp.z();
        world->moveObject(ptr, refPos.pos[0], refPos.pos[1], refPos.pos[2]);
    }

    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = dir.pos[0];
    move->mPosition[1] = dir.pos[1];
    move->mPosition[2] = dir.pos[2];

    world->rotateObject(ptr, pos.rot[0], pos.rot[1], pos.rot[2]);
}

void Players::Update(float dt)
{
    static float timer = 0;
    timer += dt;
    for (std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin(); it != players.end(); it++)
    {
        DedicatedPlayer *pl = it->second;
        if (pl == 0) continue;

        MWMechanics::NpcStats *ptrNpcStats = &pl->ptr.getClass().getNpcStats(pl->getPtr());

        MWMechanics::DynamicStat<float> value;

        if (pl->CreatureStats()->mDead)
        {
            value.readState(pl->CreatureStats()->mDynamic[0]);
            ptrNpcStats->setHealth(value);
            continue;
        }

        value.readState(pl->CreatureStats()->mDynamic[0]);
        ptrNpcStats->setHealth(value);
        value.readState(pl->CreatureStats()->mDynamic[1]);
        ptrNpcStats->setMagicka(value);
        value.readState(pl->CreatureStats()->mDynamic[2]);
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
        pl->Move(dt);
        pl->UpdateDrawState();

        if (timer >= 0.2) // call every 200 msec
            pl->updateMarker();
    }
    if (timer >= 0.2)
        timer = 0;
}

void DedicatedPlayer::UpdatePtr(MWWorld::Ptr newPtr)
{
    ptr.mCell = newPtr.mCell;
    ptr.mRef = newPtr.mRef;
    ptr.mContainerStore = newPtr.mContainerStore;

    // Disallow this player's reference from moving across cells until
    // the correct packet is sent by the player
    ptr.getBase()->canChangeCell = false;
}


DedicatedPlayer *Players::NewPlayer(RakNet::RakNetGUID guid)
{
    LOG_APPEND(Log::LOG_INFO, "- Creating new DedicatedPlayer with guid %i",
        guid.ToUint32);

    players[guid] = new DedicatedPlayer(guid);
    players[guid]->state = 0;
    return players[guid];
}

void DedicatedPlayer::UpdateInventory()
{
    MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);

        const string &dedicItem = EquipedItem(slot)->refid;
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

        const int count = EquipedItem(slot)->count;
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

const std::string DedicatedPlayer::GetAnim()
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

DedicatedPlayer *Players::GetPlayer(const MWWorld::Ptr &ptr)
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

void DedicatedPlayer::UpdateDrawState()
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

    if (cell.isExterior() == 1)
        cellStore = world->getExterior(cell.mCellId.mIndex.mX, cell.mCellId.mIndex.mY);
    else if (!cell.mName.empty())
        cellStore = world->getInterior(cell.mName);
    // Go no further if cell data is invalid
    else
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Server sent invalid cell change info about %s (%s)!",
            ptr.getBase()->mRef.getRefId().c_str(),
            this->Npc()->mName.c_str());

        return;
    }

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Server says %s (%s) moved to %s",
        ptr.getBase()->mRef.getRefId().c_str(),
        this->Npc()->mName.c_str(),
        cellStore->getCell()->getDescription().c_str());

    // Allow this player's reference to move across a cell now that
    // a manual cell update has been called
    ptr.getBase()->canChangeCell = true;
    UpdatePtr(world->moveObject(ptr, cellStore, pos.pos[0], pos.pos[1], pos.pos[2]));
}


void DedicatedPlayer::updateMarker()
{
    if (!markerEnabled)
        return;
    GUIController *gui = Main::get().getGUIController();
    if (gui->mPlayerMarkers.isExists(marker))
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

void DedicatedPlayer::enableMarker(bool enable)
{
    if (enable)
        updateMarker();
    else
        removeMarker();
}
