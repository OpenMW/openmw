#ifndef OPENMW_PACKETACTORDYNAMICSTATS_HPP
#define OPENMW_PACKETACTORDYNAMICSTATS_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorDynamicStats : public ActorPacket
    {
    public:
        PacketActorDynamicStats(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORDYNAMICSTATS_HPP
