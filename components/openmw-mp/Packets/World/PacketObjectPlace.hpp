#ifndef OPENMW_PACKETOBJECTPLACE_HPP
#define OPENMW_PACKETOBJECTPLACE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectPlace : public WorldPacket
    {
    public:
        PacketObjectPlace(RakNet::RakPeerInterface *peer);

        virtual void Object(WorldObject &worldObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTPLACE_HPP
