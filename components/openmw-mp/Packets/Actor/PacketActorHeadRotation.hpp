#ifndef OPENMW_PACKETACTORHEADROTATION_HPP
#define OPENMW_PACKETACTORHEADROTATION_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorHeadRotation : public ActorPacket
    {
    public:
        PacketActorHeadRotation(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORHEADROTATION_HPP
