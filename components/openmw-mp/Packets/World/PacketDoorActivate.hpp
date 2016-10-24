#ifndef OPENMW_PACKETDOORACTIVATE_HPP
#define OPENMW_PACKETDOORACTIVATE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketDoorActivate : public WorldPacket
    {
    public:
        PacketDoorActivate(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, WorldEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETDOORACTIVATE_HPP
