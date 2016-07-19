//
// Created by koncord on 05.01.16.
//

#include "Player.hpp"

TPlayers Players::players;
TSlots Players::slots;

void Players::DeletePlayer(RakNet::RakNetGUID id)
{
    if(players[id] != 0)
    {
        slots[players[id]->GetID()] = 0;
        delete players[id];
        players.erase(id);
    }

}

void Players::NewPlayer(RakNet::RakNetGUID id)
{
    players[id] = new Player(id);

    for(int i = 0; i < 16; i++)
    {
        if(slots[i] == 0)
        {
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
