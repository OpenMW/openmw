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
typedef std::map<uint64_t, Player*> TPlayers;
typedef std::map<unsigned short, Player*> TSlots;

class Players
{
public:
    static void NewPlayer(RakNet::RakNetGUID guid);
    static void DeletePlayer(RakNet::RakNetGUID guid);
    static Player *GetPlayer(RakNet::RakNetGUID guid);
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

    enum
    {
        NOTLOADED=0,
        LOADED,
        POSTLOADED
    };
    Player(RakNet::RakNetGUID guid);

    unsigned short GetID();
    void SetID(unsigned short id);

    bool isHandshaked();
    void Handshake();

    void Loaded(int state);
    int LoadedState();

    void setLastAttackerID(unsigned short pid);
    void resetLastAttacker();
    unsigned short getLastAttackerID();

    virtual ~Player();
public:
    mwmp::Inventory inventorySendBuffer;
private:
    bool handshake;
    int loaded;
    unsigned short lastAttacker;
};

#endif //OPENMW_PLAYER_HPP
