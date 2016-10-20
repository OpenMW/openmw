#ifndef OPENMW_PACKETOBJECTREMOVAL_HPP
#define OPENMW_PACKETOBJECTREMOVAL_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectRemoval : public WorldPacket
    {
    public:
        PacketObjectRemoval(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTREMOVAL_HPP
