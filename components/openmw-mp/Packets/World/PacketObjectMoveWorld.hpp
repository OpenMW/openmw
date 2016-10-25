#ifndef OPENMW_PACKETOBJECTMOVEWORLD_HPP
#define OPENMW_PACKETOBJECTMOVEWORLD_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectMoveWorld : public WorldPacket
    {
    public:
        PacketObjectMoveWorld(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTMOVEWORLD_HPP
