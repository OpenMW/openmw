#ifndef OPENMW_PACKETOBJECTSCALE_HPP
#define OPENMW_PACKETOBJECTSCALE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectScale : public WorldPacket
    {
    public:
        PacketObjectScale(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTSCALE_HPP
