//
// Created by koncord on 04.01.16.
//

#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <RakPeerInterface.h>
#include <BitStream.h>
#include <string>

#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Packets/Player/PacketPosition.hpp>
#include <components/openmw-mp/Packets/Player/PacketBaseInfo.hpp>
#include <components/openmw-mp/Packets/Player/PacketEquipment.hpp>
#include <components/openmw-mp/Packets/Player/PacketAttack.hpp>
#include <components/openmw-mp/Packets/Player/PacketDynamicStats.hpp>
#include <components/openmw-mp/Packets/Player/PacketResurrect.hpp>
#include <components/openmw-mp/Packets/Player/PacketDie.hpp>
#include <components/openmw-mp/Packets/Player/PacketCell.hpp>
#include <components/openmw-mp/Packets/Player/PacketDrawState.hpp>
#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>

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
        PlayerPacket *GetPacket(RakNet::MessageID id);

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

        PlayerPacketController controller;

        void ReceiveMessage(RakNet::Packet *packet);
        LocalPlayer *getLocalPlayer();
    };
}


#endif //OPENMW_NETWORKING_HPP
