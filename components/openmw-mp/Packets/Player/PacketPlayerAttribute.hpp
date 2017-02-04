//
// Created by koncord on 08.03.16.
//

#ifndef OPENMW_PACKETPLAYERATTRIBUTE_HPP
#define OPENMW_PACKETPLAYERATTRIBUTE_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerAttribute : public PlayerPacket
    {
    public:
        const static int AttributeCount = 8;
        PacketPlayerAttribute(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERATTRIBUTE_HPP
