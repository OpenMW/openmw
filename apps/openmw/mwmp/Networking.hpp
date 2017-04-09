//
// Created by koncord on 04.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <RakPeerInterface.h>
#include <BitStream.h>
#include <string>

#include "ActorList.hpp"
#include "WorldEvent.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>
#include <components/openmw-mp/Controllers/ActorPacketController.hpp>
#include <components/openmw-mp/Controllers/WorldPacketController.hpp>

#include <components/files/collections.hpp>

namespace mwmp
{
    class LocalPlayer;

    class Networking
    {
    public:
        Networking();
        ~Networking();
        void connect(const std::string& ip, unsigned short port, std::vector<std::string> &content, Files::Collections &collections);
        void update();

        PlayerPacket *getPlayerPacket(RakNet::MessageID id);
        ActorPacket *getActorPacket(RakNet::MessageID id);
        WorldPacket *getWorldPacket(RakNet::MessageID id);

        bool isDedicatedPlayer(const MWWorld::Ptr &ptr);
        bool attack(const MWWorld::Ptr &ptr);

        RakNet::SystemAddress serverAddress()
        {
            return serverAddr;
        }

        bool isConnected();

        LocalPlayer *getLocalPlayer();
        ActorList *getActorList();
        WorldEvent *getWorldEvent();

    private:
        bool connected;
        RakNet::RakPeerInterface *peer;
        RakNet::SystemAddress serverAddr;
        RakNet::BitStream bsOut;

        PlayerPacketController playerPacketController;
        ActorPacketController actorPacketController;
        WorldPacketController worldPacketController;

        ActorList actorList;
        WorldEvent worldEvent;

        void processPlayerPacket(RakNet::Packet *packet);
        void processActorPacket(RakNet::Packet *packet);
        void processWorldPacket(RakNet::Packet *packet);

        void receiveMessage(RakNet::Packet *packet);

        void preInit(std::vector<std::string> &content, Files::Collections &collections);
    };
}


#endif //OPENMW_NETWORKING_HPP
