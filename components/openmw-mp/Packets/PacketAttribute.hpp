//
// Created by koncord on 08.03.16.
//

#ifndef OPENMW_PACKETATTRIBUTE_HPP
#define OPENMW_PACKETATTRIBUTE_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketAttribute : public BasePacket
    {
    public:
        const static int AttributesCount = 8;
        PacketAttribute(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETATTRIBUTE_HPP
