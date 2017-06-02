#ifndef OPENMW_PACKETOBJECTTRAP_HPP
#define OPENMW_PACKETOBJECTTRAP_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectTrap : public WorldPacket
    {
    public:
        PacketObjectTrap(RakNet::RakPeerInterface *peer);

        virtual void Object(WorldObject &worldObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTTRAP_HPP
