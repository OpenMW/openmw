#ifndef OPENMW_PACKETACTORPOSITION_HPP
#define OPENMW_PACKETACTORPOSITION_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorPosition : public ActorPacket
    {
    public:
        PacketActorPosition(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORPOSITION_HPP
