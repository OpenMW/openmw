//
// Created by koncord on 05.01.16.
//

#ifndef OPENMW_PLAYER_HPP
#define OPENMW_PLAYER_HPP

#include <map>
#include <string>
#include <RakNetTypes.h>

#include <components/esm/npcstats.hpp>
#include <components/esm/cellid.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadcell.hpp>

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>

struct Player;
typedef std::map<RakNet::RakNetGUID, Player*> TPlayers;
typedef std::map<unsigned short, Player*> TSlots;

class Players
{
public:
    static void NewPlayer(RakNet::RakNetGUID id);
    static void DeletePlayer(RakNet::RakNetGUID id);
    static Player *GetPlayer(RakNet::RakNetGUID id);
    static Player *GetPlayer(unsigned short id);
    static TPlayers *GetPlayers();

private:
    static TPlayers players;
    static TSlots slots;
};

class Player : public mwmp::BasePlayer
{
    unsigned short id;
public:

    Player(RakNet::RakNetGUID id);

    unsigned short GetID();
    void SetID(unsigned short id);

    bool isHandshaked();

    void Handshake();

    virtual ~Player();
private:
    bool handshake;
};

#endif //OPENMW_PLAYER_HPP
