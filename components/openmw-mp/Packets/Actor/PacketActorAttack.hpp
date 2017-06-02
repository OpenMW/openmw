#ifndef OPENMW_PACKETACTORATTACK_HPP
#define OPENMW_PACKETACTORATTACK_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorAttack : public ActorPacket
    {
    public:
        PacketActorAttack(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORATTACK_HPP
