//
// Created by David Cernat on 25.09.16.
//

#ifndef OPENMW_PACKETLEVEL_HPP
#define OPENMW_PACKETLEVEL_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class PacketLevel : public BasePacket
    {
    public:
        PacketLevel(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETLEVEL_HPP
