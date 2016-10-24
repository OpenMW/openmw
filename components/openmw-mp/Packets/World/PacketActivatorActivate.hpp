#ifndef OPENMW_PACKETACTIVATORACTIVATE_HPP
#define OPENMW_PACKETACTIVATORACTIVATE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketActivatorActivate : public WorldPacket
    {
    public:
        PacketActivatorActivate(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETACTIVATORACTIVATE_HPP
