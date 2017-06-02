#ifndef OPENMW_PACKETACTORANIMFLAGS_HPP
#define OPENMW_PACKETACTORANIMFLAGS_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorAnimFlags : public ActorPacket
    {
    public:
        PacketActorAnimFlags(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORANIMFLAGS_HPP
