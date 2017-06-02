#ifndef OPENMW_PACKETACTORSTATSDYNAMIC_HPP
#define OPENMW_PACKETACTORSTATSDYNAMIC_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorStatsDynamic : public ActorPacket
    {
    public:
        PacketActorStatsDynamic(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORSTATSDYNAMIC_HPP
