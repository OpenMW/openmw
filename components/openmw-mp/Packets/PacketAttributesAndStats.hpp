//
// Created by koncord on 11.01.16.
//

#ifndef OPENMW_PacketAttributesAndStats_HPP
#define OPENMW_PacketAttributesAndStats_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketAttributesAndStats : public BasePacket
    {
    public:
        const static int AttributesCount = 8;
        const static int StatsCount = 27;
        PacketAttributesAndStats(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PacketAttributesAndStats_HPP
