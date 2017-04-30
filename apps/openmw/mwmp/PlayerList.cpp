#include <components/openmw-mp/Log.hpp>

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

std::map<RakNet::RakNetGUID, DedicatedPlayer *> PlayerList::players;

void PlayerList::update(float dt)
{
    for (std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin(); it != players.end(); it++)
    {
        DedicatedPlayer *player = it->second;
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
        if(tmpCreature == 0)
        {
            dedicPlayer->creatureModel = "";
            createPlayer(guid);
            return;
        }
        creature = *tmpCreature;
        creature.mScript = "";
        if(!dedicPlayer->useCreatureName)
            creature.mName = dedicPlayer->npc.mName;
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

    if (dedicPlayer->state == 0)
    {
        string recid;
        if (dedicPlayer->creatureModel.empty())
        {
            npc.mId = "Dedicated Player";
            recid = world->createRecord(npc)->mId;
        }
        else
        {
            creature.mId = "Dedicated Player";
            recid = world->createRecord(creature)->mId;
        }

        dedicPlayer->reference = new MWWorld::ManualRef(world->getStore(), recid, 1);
    }

    // Temporarily spawn or move player to the center of exterior 0,0 whenever setting base info
    ESM::Position spawnPos;
    spawnPos.pos[0] = spawnPos.pos[1] = Main::get().getCellController()->getCellSize() / 2;
    spawnPos.pos[2] = 0;
    MWWorld::CellStore *cellStore = world->getExterior(0, 0);

    if (dedicPlayer->state == 0)
    {
        LOG_APPEND(Log::LOG_INFO, "- Creating new reference pointer for %s", dedicPlayer->npc.mName.c_str());

        MWWorld::Ptr tmp = world->placeObject(dedicPlayer->reference->getPtr(), cellStore, spawnPos);

        dedicPlayer->ptr.mCell = tmp.mCell;
        dedicPlayer->ptr.mRef = tmp.mRef;

        dedicPlayer->cell = *dedicPlayer->ptr.getCell()->getCell();
        dedicPlayer->position = dedicPlayer->ptr.getRefData().getPosition();
    }
    else
    {
        LOG_APPEND(Log::LOG_INFO, "- Updating reference pointer for %s", dedicPlayer->npc.mName.c_str());

        MWWorld::ESMStore *store = const_cast<MWWorld::ESMStore *>(&world->getStore());

        if (!dedicPlayer->creatureModel.empty())
        {
            creature.mId = players[guid]->ptr.get<ESM::Creature>()->mBase->mId;
            MWWorld::Store<ESM::Creature> *esm_store = const_cast<MWWorld::Store<ESM::Creature> *> (&store->get<ESM::Creature>());
            esm_store->insert(creature);

        }
        else
        {
            npc.mId = players[guid]->ptr.get<ESM::NPC>()->mBase->mId;
            MWWorld::Store<ESM::NPC> *esm_store = const_cast<MWWorld::Store<ESM::NPC> *> (&store->get<ESM::NPC>());
            esm_store->insert(npc);
        }

        // Disable Ptr to avoid graphical glitches caused by race changes
        world->disable(players[guid]->ptr);
        
        dedicPlayer->setPtr(world->moveObject(dedicPlayer->ptr, cellStore, spawnPos.pos[0], spawnPos.pos[1], spawnPos.pos[2]));
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
    for (std::map <RakNet::RakNetGUID, DedicatedPlayer *>::iterator it = players.begin(); it != players.end(); it++)
        delete it->second;
}

DedicatedPlayer *PlayerList::getPlayer(RakNet::RakNetGUID guid)
{
    return players[guid];
}

DedicatedPlayer *PlayerList::getPlayer(const MWWorld::Ptr &ptr)
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

bool PlayerList::isDedicatedPlayer(const MWWorld::Ptr &ptr)
{
    if (ptr.mRef == NULL)
        return false;

    return (getPlayer(ptr) != 0);
}
