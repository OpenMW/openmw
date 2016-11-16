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

        void newPlayer(RakNet::RakNetGUID guid);
        void disconnectPlayer(RakNet::RakNetGUID guid);
        void kickPlayer(RakNet::RakNetGUID guid);

        void processPlayerPacket(RakNet::Packet *packet);
        void processWorldPacket(RakNet::Packet *packet);
        void update(RakNet::Packet *packet);

        unsigned short numberOfConnections() const;
        unsigned int maxConnections() const;
        int getAvgPing(RakNet::AddressOrGUID) const;

        int mainLoop();

        void stopServer(int code);

        PlayerPacketController *getPlayerController() const;
        WorldPacketController *getWorldController() const;

        static const Networking &get();
        static Networking *getPtr();

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
