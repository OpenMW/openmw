//
// Created by koncord on 15.01.16.
//

#ifndef OPENMW_PACKETSCONTROLLER_HPP
#define OPENMW_PACKETSCONTROLLER_HPP


#include <RakPeerInterface.h>
#include "Packets/BasePacket.hpp"

namespace mwmp
{
    class PacketPosition;
    class PacketCell;
    class PacketBaseInfo;
    class PacketEquiped;
    class PacketAttributesAndStats;
    class PacketAttack;
    class PacketMainStats;
    class PacketResurrect;
    class PacketDie;
    class PacketDrawState;

    class PacketSendMyID;
    class PacketDisconnect;

    class PacketChatMessage;
    class PacketCharGen;

    class PacketAttribute;
    class PacketSkill;
    class PacketHandshake;

    class PacketsController
    {
    public:
        PacketsController(RakNet::RakPeerInterface *peer);

        BasePacket *GetPacket(RakNet::MessageID id);

        void SetStream(RakNet::BitStream *inStream, RakNet::BitStream *outStream);

    private:
        PacketPosition *packetPosition;
        PacketCell *packetCell;
        PacketBaseInfo *packetBaseInfo;
        PacketEquiped *packetEquiped;
        PacketAttributesAndStats *attributesAndStats;
        PacketAttack *packetAttack;
        PacketMainStats *packetMainStats;
        PacketResurrect *packetResurrect;
        PacketDie *packetDie;
        PacketDrawState *packetDrawState;

        PacketSendMyID *packetSendMyID;
        PacketDisconnect *packetDisconnect;

        PacketChatMessage *packetChatMessage;
        PacketCharGen *packetCharGen;
        PacketAttribute *packetAttribute;
        PacketSkill *packetSkill;
        PacketHandshake *packetHandshake;
    };
}

#endif //OPENMW_PACKETSCONTROLLER_HPP
