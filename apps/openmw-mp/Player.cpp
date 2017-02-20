//
// Created by koncord on 05.01.16.
//

#include "Player.hpp"
#include "Networking.hpp"

TPlayers Players::players;
TSlots Players::slots;

void Players::deletePlayer(RakNet::RakNetGUID guid)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Deleting player with guid %lu",
        guid.g);

    if (players[guid] != 0)
    {
        CellController::get()->deletePlayer(players[guid]);

        LOG_APPEND(Log::LOG_INFO, "- Emptying slot %i",
            players[guid]->getId());

        slots[players[guid]->getId()] = 0;
        delete players[guid];
        players.erase(guid);
    }
}

void Players::newPlayer(RakNet::RakNetGUID guid)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Creating new player with guid %lu",
        guid.g);

    players[guid] = new Player(guid);
    players[guid]->cell.blank();
    players[guid]->npc.blank();
    players[guid]->npcStats.blank();
    players[guid]->creatureStats.blank();
    players[guid]->charClass.blank();

    for (unsigned int i = 0; i < mwmp::Networking::get().maxConnections(); i++)
    {
        if (slots[i] == 0)
        {
            LOG_APPEND(Log::LOG_INFO, "- Storing in slot %i",
                i);

            slots[i] = players[guid];
            slots[i]->setId(i);
            break;
        }
    }
}

Player *Players::getPlayer(RakNet::RakNetGUID guid)
{
    if (players.count(guid) == 0)
        return nullptr;
    return players[guid];
}

TPlayers *Players::getPlayers()
{
    return &players;
}

Player::Player(RakNet::RakNetGUID guid) : BasePlayer(guid)
{
    handshakeState = false;
    loadState = NOTLOADED;
    lastAttacker = 0;
}

Player::~Player()
{

}

unsigned short Player::getId()
{
    return id;
}

void Player::setId(unsigned short id)
{
    this->id = id;
}

void Player::setHandshake()
{
    handshakeState = true;
}

bool Player::isHandshaked()
{
    return handshakeState;
}

void Player::setLoadState(int state)
{
    loadState = state;
}

int Player::getLoadState()
{
    return loadState;
}

Player *Players::getPlayer(unsigned short id)
{
    if (slots.find(id) == slots.end())
        return nullptr;
    return slots[id];
}

void Player::setLastAttackerId(unsigned short pid)
{
    lastAttacker = pid;
}

void Player::resetLastAttacker()
{
    lastAttacker = id;
}

unsigned short Player::getLastAttackerId()
{
    return lastAttacker;
}

void Player::setLastAttackerTime(std::chrono::steady_clock::time_point time)
{
    lastAttackerTime = time;
}

std::chrono::steady_clock::time_point Player::getLastAttackerTime()
{
    return lastAttackerTime;
}

CellController::TContainer *Player::getCells()
{
    return &cells;
}

void Player::sendToLoaded(mwmp::PlayerPacket *myPacket)
{
    std::list <Player*> plList;

    for (auto cell : cells)
        for (auto pl : *cell)
            plList.push_back(pl);

    plList.sort();
    plList.unique();

    for (auto pl : plList)
    {
        if (pl == this) continue;
        myPacket->Send(this, pl->guid);
    }
}

void Player::forEachLoaded(std::function<void(Player *pl, Player *other)> func)
{
    std::list <Player*> plList;

    for (auto cell : cells)
        for (auto pl : *cell)
            plList.push_back(pl);

    plList.sort();
    plList.unique();

    for (auto pl : plList)
    {
        if (pl == this) continue;
        func(this, pl);
    }
}
