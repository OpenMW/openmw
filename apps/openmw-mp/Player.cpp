//
// Created by koncord on 05.01.16.
//

#include "Player.hpp"
#include "Networking.hpp"

TPlayers Players::players;
TSlots Players::slots;

void Players::DeletePlayer(RakNet::RakNetGUID guid)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Deleting player with guid %lu",
        guid.g);

    if (players[guid] != 0)
    {
        LOG_APPEND(Log::LOG_INFO, "- Emptying slot %i",
            players[guid]->GetID());

        slots[players[guid]->GetID()] = 0;
        delete players[guid];
        players.erase(guid);
    }

    LOG_APPEND(Log::LOG_INFO, "- %i remaining",
        players.size());
}

void Players::NewPlayer(RakNet::RakNetGUID guid)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Creating new player with guid %lu",
        guid.g);

    players[guid] = new Player(guid);
    players[guid]->GetCell()->blank();
    players[guid]->Npc()->blank();
    players[guid]->NpcStats()->blank();
    players[guid]->CreatureStats()->blank();
    players[guid]->charClass.blank();

    for (int i = 0; i < mwmp::Networking::Get().MaxConnections(); i++)
    {
        if (slots[i] == 0)
        {
            LOG_APPEND(Log::LOG_INFO, "- Storing in slot %i",
                i);

            slots[i] = players[guid];
            slots[i]->SetID(i);
            break;
        }
    }
}

Player *Players::GetPlayer(RakNet::RakNetGUID guid)
{
    return players[guid];
}

std::map<RakNet::RakNetGUID, Player*> *Players::GetPlayers()
{
    return &players;
}

Player::Player(RakNet::RakNetGUID guid) : BasePlayer(guid)
{
    handshake = false;
    loaded = false;
    lastAttacker = 0;
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

void Player::Loaded(int state)
{
    loaded = state;
}

int Player::LoadedState()
{
    return loaded;
}

Player *Players::GetPlayer(unsigned short id)
{
    if (slots.find(id) == slots.end())
        return nullptr;
    return slots[id];
}

void Player::setLastAttackerID(unsigned short pid)
{
    lastAttacker = pid;
}

void Player::resetLastAttacker()
{
    lastAttacker = id;
}

unsigned short Player::getLastAttackerID()
{
    return lastAttacker;
}
