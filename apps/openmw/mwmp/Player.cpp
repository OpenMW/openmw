//
// Created by koncord on 02.01.16.
//

#include <bits/stringfwd.h>
#include <apps/openmw/mwmechanics/aitravel.hpp>
#include "../mwbase/environment.hpp"
#include "../mwstate/statemanagerimp.hpp"
#include "../mwinput/inputmanagerimp.hpp"
#include "../mwgui/windowmanagerimp.hpp"
#include "../mwworld/worldimp.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwclass/npc.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "Player.hpp"

using namespace mwmp;

std::map<int, Player *> Player::players;

Player::~Player()
{
    delete reference;
}

MWWorld::Ptr Player::getPtr()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    return world->getPtr(reference->getPtr().get<ESM::NPC>()->mBase->mId, false);
}

Player::Player()
{

}

void Player::CreatePlayer(int id, const std::string &name, const std::string &race, const std::string &head,
                          const std::string &hair)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWWorld::Ptr player = world->getPlayerPtr();

    ESM::NPC dedic_pl = *player.get<ESM::NPC>()->mBase;
    dedic_pl.mRace = race;
    dedic_pl.mHead = head;
    dedic_pl.mHair = hair;
    dedic_pl.mName = name;


    if (players[id] == 0)
    {
        dedic_pl.mId = "Dedicated Player";

        std::string recid = world->createRecord(dedic_pl)->mId;

        players[id] = new Player();
        Player *_player = players[id];

        _player->reference = new MWWorld::ManualRef(world->getStore(), recid, 1);

        // temporary spawn character in ToddTest cell
        ESM::Position pos;
        world->findInteriorPosition("ToddTest", pos);
        MWWorld::CellStore *store = world->getInterior("ToddTest");

        MWWorld::Ptr tmp = world->safePlaceObject(_player->reference->getPtr(), store, pos);
        _player->ptr = world->getPtr(tmp.get<ESM::NPC>()->mBase->mId, false);

    }
    else
    {
        dedic_pl.mId = players[id]->reference->getPtr().get<ESM::NPC>()->mBase->mId;

        MWWorld::ESMStore *store = const_cast<MWWorld::ESMStore *>(&world->getStore());
        MWWorld::Store<ESM::NPC> *esm_store = const_cast<MWWorld::Store<ESM::NPC> *> (&store->get<ESM::NPC>());

        esm_store->insert(dedic_pl);

    }

    players[id]->active = true;

    world->enable(players[id]->reference->getPtr());
}


void Player::CleanUp()
{
    for(std::map <int, Player*>::iterator it = players.begin(); it != players.end(); it++)
        delete it->second;
}

void Player::DestroyPlayer(int id)
{
    if (players[id]->active)
    {
        players[id]->active = false;
        MWBase::World *world = MWBase::Environment::get().getWorld();
        world->disable(players[id]->getPtr());

        //move player to toddTest
        ESM::Position pos;
        world->findInteriorPosition("ToddTest", pos);
        MWWorld::CellStore *store = world->getInterior("ToddTest");

        players[id]->ptr = world->moveObject(players[id]->getPtr(), store, pos.pos[0], pos.pos[1], pos.pos[2]);
    }
}

Player *Player::GetPlayer(int id)
{
    return players[id];
}

MWWorld::Ptr Player::getLiveCellPtr()
{
    return reference->getPtr();
}

MWWorld::ManualRef *Player::getRef()
{
    return reference;
}

void Player::Move(ESM::Position pos, MWWorld::CellStore *cell)
{
    if (!active) return;
    MWWorld::Ptr myPtr = getPtr();
    ESM::Position ref_pos = myPtr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();

    float xx = pos.pos[0] - ref_pos.pos[0];
    float yy = pos.pos[1] - ref_pos.pos[1];
    double d = sqrt((xx * xx) + (yy * yy));

    MWMechanics::AiSequence *aiSequence = &myPtr.getClass().getCreatureStats(myPtr).getAiSequence();

    if (d > 10.0 && d < 150.0)
    {
        MWMechanics::AiTravel travelPackage(pos.pos[0], pos.pos[1], pos.pos[2]);
        aiSequence->clear();
        aiSequence->stack(travelPackage, myPtr);
    }
    else if (d == 0.0)
        aiSequence->clear();
    else if (d >= 150.0)
        world->moveObject(myPtr, cell, pos.pos[0], pos.pos[1], pos.pos[2]);

    world->rotateObject(myPtr, pos.rot[0], pos.rot[1], pos.rot[2]);
}
