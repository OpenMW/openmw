#ifndef OPENMW_PACKETACTORFRAME_HPP
#define OPENMW_PACKETACTORFRAME_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketActorFrame : public WorldPacket
    {
    public:
        PacketActorFrame(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORFRAME_HPP
