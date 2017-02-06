#ifndef OPENMW_PACKETDOORSTATE_HPP
#define OPENMW_PACKETDOORSTATE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketDoorState : public WorldPacket
    {
    public:
        PacketDoorState(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETDOORSTATE_HPP
