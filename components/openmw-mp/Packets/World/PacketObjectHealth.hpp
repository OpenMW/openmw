#ifndef OPENMW_PACKETOBJECTHEALTH_HPP
#define OPENMW_PACKETOBJECTHEALTH_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectHealth : public WorldPacket
    {
    public:
        PacketObjectHealth(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTHEALTH_HPP
