#ifndef OPENMW_PACKETACTORDEATH_HPP
#define OPENMW_PACKETACTORDEATH_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorDeath : public ActorPacket
    {
    public:
        PacketActorDeath(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORDEATH_HPP
