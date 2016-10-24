#ifndef OPENMW_PACKETCONTAINERREMOVE_HPP
#define OPENMW_PACKETCONTAINERREMOVE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketContainerRemove : public WorldPacket
    {
    public:
        PacketContainerRemove(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETCONTAINERREMOVE_HPP
