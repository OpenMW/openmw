//
// Created by koncord on 05.01.16.
//

#include "Player.hpp"
#include "Networking.hpp"

TPlayers Players::players;
TSlots Players::slots;

void Players::DeletePlayer(RakNet::RakNetGUID id)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Deleting player with guid %s",
        id.ToString());

    if (players[id] != 0)
    {
        LOG_APPEND(Log::LOG_INFO, "- Emptying slot %i",
            players[id]->GetID());

        slots[players[id]->GetID()] = 0;
        delete players[id];
        players.erase(id);
    }

}

void Players::NewPlayer(RakNet::RakNetGUID id)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Creating new player with guid %s",
        id.ToString());

    players[id] = new Player(id);
    players[id]->GetCell()->blank();
    players[id]->Npc()->blank();
    players[id]->NpcStats()->blank();
    players[id]->CreatureStats()->blank();
    players[id]->klass.blank();

    for (int i = 0; i < mwmp::Networking::Get().MaxConnections(); i++)
    {
        if (slots[i] == 0)
        {
            LOG_APPEND(Log::LOG_INFO, "- Storing in slot %i",
                i);

            slots[i] = players[id];
            slots[i]->SetID(i);
            break;
        }
    }
}

Player *Players::GetPlayer(RakNet::RakNetGUID id)
{
    return players[id];
}

std::map<RakNet::RakNetGUID, Player*> *Players::GetPlayers()
{
    return &players;
}

Player::Player(RakNet::RakNetGUID id) : BasePlayer(id)
{
    handshake = false;
}

Player::~Player()
{

}

unsigned short Player::GetID()
{
    return id;
}

void Player::SetID(unsigned short id)
{
    this->id = id;
}

void Player::Handshake()
{
    handshake = true;
}

bool Player::isHandshaked()
{
    return handshake;
}


Player *Players::GetPlayer(unsigned short id)
{
    return slots[id];
}
