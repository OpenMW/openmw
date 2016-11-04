//
// Created by koncord on 12.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>
#include <components/openmw-mp/Controllers/WorldPacketController.hpp>
#include "Player.hpp"

namespace  mwmp
{
    class Networking
    {
    public:
        Networking(RakNet::RakPeerInterface *peer);
        ~Networking();

        void NewPlayer(RakNet::RakNetGUID guid);
        void DisconnectPlayer(RakNet::RakNetGUID guid);
        void KickPlayer(RakNet::RakNetGUID guid);

        void ProcessPlayerPacket(RakNet::Packet *packet);
        void ProcessWorldPacket(RakNet::Packet *packet);
        void Update(RakNet::Packet *packet);

        unsigned short NumberOfConnections() const;
        unsigned int MaxConnections() const;
        int GetAvgPing(RakNet::AddressOrGUID) const;

        int MainLoop();

        void StopServer(int code);

        PlayerPacketController *GetPlayerController() const;
        WorldPacketController *GetWorldController() const;

        static const Networking &Get();
        static Networking *GetPtr();

    private:
        static Networking *sThis;
        RakNet::RakPeerInterface *peer;
        RakNet::BitStream bsOut;
        TPlayers *players;

        PlayerPacketController *playerController;
        WorldPacketController *worldController;

        bool running;
        int exitCode;
    };
}


#endif //OPENMW_NETWORKING_HPP
