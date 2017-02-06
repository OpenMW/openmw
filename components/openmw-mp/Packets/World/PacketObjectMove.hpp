#ifndef OPENMW_PacketObjectMove_HPP
#define OPENMW_PacketObjectMove_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectMove : public WorldPacket
    {
    public:
        PacketObjectMove(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PacketObjectMove_HPP
