#ifndef OPENMW_PACKETCONTAINERADD_HPP
#define OPENMW_PACKETCONTAINERADD_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketContainerAdd : public WorldPacket
    {
    public:
        PacketContainerAdd(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETCONTAINERADD_HPP
