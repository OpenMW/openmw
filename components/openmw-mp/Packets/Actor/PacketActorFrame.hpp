#ifndef OPENMW_PACKETACTORFRAME_HPP
#define OPENMW_PACKETACTORFRAME_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorFrame : public ActorPacket
    {
    public:
        PacketActorFrame(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORFRAME_HPP
