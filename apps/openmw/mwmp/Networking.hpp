//
// Created by koncord on 04.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <RakPeerInterface.h>
#include <BitStream.h>
#include <string>

#include "WorldEvent.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>
#include <components/openmw-mp/Controllers/WorldPacketController.hpp>

namespace mwmp
{
    class LocalPlayer;

    class Networking
    {
    public:
        Networking();
        ~Networking();
        void connect(const std::string& ip, unsigned short port);
        void update();
        void sendData(RakNet::BitStream *bitStream);

        PlayerPacket *getPlayerPacket(RakNet::MessageID id);
        WorldPacket *getWorldPacket(RakNet::MessageID id);

        bool isDedicatedPlayer(const MWWorld::Ptr &ptr);
        bool attack(const MWWorld::Ptr &ptr);

        RakNet::SystemAddress serverAddress()
        {
            return serverAddr;
        }

        bool isConnected();

        WorldEvent *resetWorldEvent();

    private:
        bool connected;
        RakNet::RakPeerInterface *peer;
        RakNet::SystemAddress serverAddr;
        RakNet::BitStream bsOut;

        PlayerPacketController playerController;
        WorldPacketController worldController;
        WorldEvent worldEvent;

        void processPlayerPacket(RakNet::Packet *packet);
        void processWorldPacket(RakNet::Packet *packet);
        void receiveMessage(RakNet::Packet *packet);
        LocalPlayer *getLocalPlayer();
    };
}


#endif //OPENMW_NETWORKING_HPP
