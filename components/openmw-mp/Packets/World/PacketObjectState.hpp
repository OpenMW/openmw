#ifndef OPENMW_PACKETOBJECTSTATE_HPP
#define OPENMW_PACKETOBJECTSTATE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectState : public WorldPacket
    {
    public:
        PacketObjectState(RakNet::RakPeerInterface *peer);
    };
}

#endif //OPENMW_PACKETOBJECTSTATE_HPP
