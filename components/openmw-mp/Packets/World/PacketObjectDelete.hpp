#ifndef OPENMW_PACKETOBJECTDELETE_HPP
#define OPENMW_PACKETOBJECTDELETE_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectDelete : public WorldPacket
    {
    public:
        PacketObjectDelete(RakNet::RakPeerInterface *peer);
    };
}

#endif //OPENMW_PACKETOBJECTDELETE_HPP
