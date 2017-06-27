#include <components/openmw-mp/Log.hpp>
#include <apps/openmw/mwclass/creature.hpp>

#include "../mwbase/environment.hpp"

#include "../mwclass/npc.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldimp.hpp"

#include "PlayerList.hpp"
#include "Main.hpp"
#include "DedicatedPlayer.hpp"
#include "CellController.hpp"
#include "GUIController.hpp"


using namespace mwmp;
using namespace std;

std::map <RakNet::RakNetGUID, DedicatedPlayer *> PlayerList::players;

void PlayerList::update(float dt)
{
    for (auto &p : players)
    {
        DedicatedPlayer *player = p.second;
        if (player == 0) continue;

        player->update(dt);
    }
}

void PlayerList::createPlayer(RakNet::RakNetGUID guid)
{
    LOG_APPEND(Log::LOG_INFO, "- Setting up character info");

    MWBase::World *world = MWBase::Environment::get().getWorld();

    DedicatedPlayer *dedicPlayer = players[guid];

    ESM::Creature creature;
    ESM::NPC npc;
    if (!dedicPlayer->creatureModel.empty())
    {
        const ESM::Creature *tmpCreature = world->getStore().get<ESM::Creature>().search(dedicPlayer->creatureModel);
        if (tmpCreature == 0)
        {
            dedicPlayer->creatureModel = "";
            createPlayer(guid);
            return;
        }
        creature = *tmpCreature;
        creature.mScript = "";
        if (!dedicPlayer->useCreatureName)
            creature.mName = dedicPlayer->npc.mName;
        LOG_APPEND(Log::LOG_INFO, "Player %s looks like %s", dedicPlayer->npc.mName.c_str(),
                   dedicPlayer->creatureModel.c_str());
    }
    else
    {
        MWWorld::Ptr player = world->getPlayerPtr();

        npc = *player.get<ESM::NPC>()->mBase;

        // To avoid freezes caused by invalid races, only set race if we find it
        // on our client
        if (world->getStore().get<ESM::Race>().search(dedicPlayer->npc.mRace) != 0)
            npc.mRace = dedicPlayer->npc.mRace;

        npc.mHead = dedicPlayer->npc.mHead;
        npc.mHair = dedicPlayer->npc.mHair;
        npc.mClass = dedicPlayer->npc.mClass;
        npc.mName = dedicPlayer->npc.mName;
        npc.mFlags = dedicPlayer->npc.mFlags;
    }

    bool reset = false;
    if (dedicPlayer->reference)
    {
        bool isNPC = dedicPlayer->reference->getPtr().getTypeName() == typeid(ESM::NPC).name();
        if ((!dedicPlayer->creatureModel.empty() && isNPC) ||
            (dedicPlayer->creatureModel.empty() && !isNPC))
        {
            if (dedicPlayer->reference)
            {
                LOG_APPEND(Log::LOG_INFO, "- Deleting old reference");
                dedicPlayer->state = 0;
                world->deleteObject(dedicPlayer->ptr);
                delete dedicPlayer->reference;
                dedicPlayer->reference = NULL;
                reset = true;
            }
        }
    }

    // Temporarily spawn or move player to the center of exterior 0,0 whenever setting base info
    ESM::Position spawnPos;
    spawnPos.pos[0] = spawnPos.pos[1] = Main::get().getCellController()->getCellSize() / 2;
    spawnPos.pos[2] = 0;
    MWWorld::CellStore *cellStore = world->getExterior(0, 0);

    if (dedicPlayer->state == 0)
    {
        string recid;
        if (dedicPlayer->creatureModel.empty())
        {
            LOG_APPEND(Log::LOG_INFO, "- Creating new NPC record");
            npc.mId = "Dedicated Player";
            recid = world->createRecord(npc)->mId;
        }
        else
        {
            LOG_APPEND(Log::LOG_INFO, "- Creating new Creature record");
            creature.mId = "Dedicated Player";
            recid = world->createRecord(creature)->mId;
        }

        dedicPlayer->reference = new MWWorld::ManualRef(world->getStore(), recid, 1);

        LOG_APPEND(Log::LOG_INFO, "- Creating new reference pointer for %s", dedicPlayer->npc.mName.c_str());

        MWWorld::Ptr tmp;

        if (reset)
        {
            if (dedicPlayer->cell.isExterior())
                cellStore = world->getExterior(dedicPlayer->cell.mData.mX, dedicPlayer->cell.mData.mY);
            else
                cellStore = world->getInterior(dedicPlayer->cell.mName);

            spawnPos = dedicPlayer->position;
        }

        tmp = world->placeObject(dedicPlayer->reference->getPtr(), cellStore, spawnPos);

        dedicPlayer->ptr.mCell = tmp.mCell;
        dedicPlayer->ptr.mRef = tmp.mRef;

        if (!reset)
        {
            dedicPlayer->cell = *dedicPlayer->ptr.getCell()->getCell();
            dedicPlayer->position = dedicPlayer->ptr.getRefData().getPosition();
        }

        ESM::CustomMarker mEditingMarker = Main::get().getGUIController()->createMarker(guid);
        dedicPlayer->marker = mEditingMarker;
        dedicPlayer->setMarkerState(true);
    }
    else
    {
        LOG_APPEND(Log::LOG_INFO, "- Updating reference pointer for %s", dedicPlayer->npc.mName.c_str());

        MWWorld::ESMStore *store = const_cast<MWWorld::ESMStore *>(&world->getStore());
        MWWorld::Store<ESM::Creature> *creature_store = const_cast<MWWorld::Store<ESM::Creature> *> (&store->get<ESM::Creature>());
        MWWorld::Store<ESM::NPC> *npc_store = const_cast<MWWorld::Store<ESM::NPC> *> (&store->get<ESM::NPC>());

        if (!dedicPlayer->creatureModel.empty())
        {
            if (!npc.mId.empty() || npc.mId != "Dedicated Player")
            {
                LOG_APPEND(Log::LOG_INFO, "- Deleting NPC record");
                npc_store->erase(npc.mId);
                npc.mId.clear();
            }
            creature.mId = players[guid]->ptr.get<ESM::Creature>()->mBase->mId;
            creature_store->insert(creature);
        }
        else
        {
            if (!creature.mId.empty() || creature.mId != "Dedicated Player")
            {
                LOG_APPEND(Log::LOG_INFO, "- Deleting Creature record");
                creature_store->erase(creature.mId);
                creature.mId.clear();
            }
            npc.mId = players[guid]->ptr.get<ESM::NPC>()->mBase->mId;
            npc_store->insert(npc);
        }

        // Disable Ptr to avoid graphical glitches caused by race changes
        world->disable(players[guid]->ptr);

        dedicPlayer->setPtr(world->moveObject(dedicPlayer->ptr, cellStore, spawnPos.pos[0], spawnPos.pos[1], spawnPos.pos[2]));
        dedicPlayer->setCell();
    }

    dedicPlayer->guid = guid;
    dedicPlayer->state = 2;

    // Give this new character a fatigue of at least 1 so it doesn't spawn
    // on the ground
    dedicPlayer->creatureStats.mDynamic[2].mBase = 1;

    world->enable(players[guid]->ptr);
}

DedicatedPlayer *PlayerList::newPlayer(RakNet::RakNetGUID guid)
{
    LOG_APPEND(Log::LOG_INFO, "- Creating new DedicatedPlayer with guid %lu", guid.g);

    players[guid] = new DedicatedPlayer(guid);
    players[guid]->state = 0;
    return players[guid];
}

void PlayerList::disconnectPlayer(RakNet::RakNetGUID guid)
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
        newPos.pos[0] = newPos.pos[1] = Main::get().getCellController()->getCellSize() / 2;
        newPos.pos[2] = 0;
        MWWorld::CellStore *cellStore = world->getExterior(0, 0);

        world->moveObject(players[guid]->getPtr(), cellStore, newPos.pos[0], newPos.pos[1], newPos.pos[2]);
    }
}

void PlayerList::cleanUp()
{
    for (auto &p : players)
        delete p.second;
}

DedicatedPlayer *PlayerList::getPlayer(RakNet::RakNetGUID guid)
{
    return players[guid];
}

DedicatedPlayer *PlayerList::getPlayer(const MWWorld::Ptr &ptr)
{
    for (auto &p : players)
    {
        if (p.second == 0 || p.second->getPtr().mRef == 0)
            continue;
        string refid = ptr.getCellRef().getRefId();
        if (p.second->getPtr().getCellRef().getRefId() == refid)
            return p.second;
    }
    return 0;
}

bool PlayerList::isDedicatedPlayer(const MWWorld::Ptr &ptr)
{
    if (ptr.mRef == NULL)
        return false;

    return (getPlayer(ptr) != 0);
}

/*
    Go through all DedicatedPlayers checking if their mHitAttemptActorId matches this one
    and set it to -1 if it does

    This resets the combat target for a DedicatedPlayer's followers in Actors::update()
*/
void PlayerList::clearHitAttemptActorId(int actorId)
{
    for (auto &p : players)
    {
        if (p.second == 0 || p.second->getPtr().mRef == 0)
            continue;

        MWMechanics::CreatureStats &playerCreatureStats = p.second->getPtr().getClass().getCreatureStats(p.second->getPtr());

        if (playerCreatureStats.getHitAttemptActorId() == actorId)
            playerCreatureStats.setHitAttemptActorId(-1);
    }
}
