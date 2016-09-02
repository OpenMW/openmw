//
// Created by koncord on 04.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <RakPeerInterface.h>
#include <BitStream.h>
#include <string>

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/PacketPosition.hpp>
#include <components/openmw-mp/Packets/PacketBaseInfo.hpp>
#include <components/openmw-mp/Packets/PacketEquiped.hpp>
#include <components/openmw-mp/Packets/PacketAttack.hpp>
#include <components/openmw-mp/Packets/PacketMainStats.hpp>
#include <components/openmw-mp/Packets/PacketResurrect.hpp>
#include <components/openmw-mp/Packets/PacketDie.hpp>
#include <components/openmw-mp/Packets/PacketCell.hpp>
#include <components/openmw-mp/Packets/PacketDrawState.hpp>
#include <components/openmw-mp/PacketsController.hpp>

namespace mwmp
{
    class LocalPlayer;

    class Networking
    {
    public:
        Networking();
        ~Networking();
        void Connect(const std::string& ip, unsigned short port);
        void Update();
        void SendData(RakNet::BitStream *bitStream);
        BasePacket *GetPacket(RakNet::MessageID id);

        bool isDedicatedPlayer(const MWWorld::Ptr &ptr);
        bool Attack(const MWWorld::Ptr &ptr);

        RakNet::SystemAddress serverAddress()
        {
            return serverAddr;
        }

        bool isConnected();

    private:
        bool connected;
        RakNet::RakPeerInterface *peer;
        RakNet::SystemAddress serverAddr;
        RakNet::BitStream bsOut;

        PacketsController controller;

        void ReceiveMessage(RakNet::Packet *packet);
        LocalPlayer *getLocalPlayer();
    };
}


#endif //OPENMW_NETWORKING_HPP
