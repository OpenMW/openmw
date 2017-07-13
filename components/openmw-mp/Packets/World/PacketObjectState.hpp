#ifndef OPENMW_PACKETOBJECTSTATE_HPP
#define OPENMW_PACKETOBJECTSTATE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectState : public WorldPacket
    {
    public:
        PacketObjectState(RakNet::RakPeerInterface *peer);

        virtual void Object(WorldObject &worldObject, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTSTATE_HPP
