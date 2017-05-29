#ifndef OPENMW_PACKETOBJECTSPAWN_HPP
#define OPENMW_PACKETOBJECTSPAWN_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectSpawn : public WorldPacket
    {
    public:
        PacketObjectSpawn(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTSPAWN_HPP
