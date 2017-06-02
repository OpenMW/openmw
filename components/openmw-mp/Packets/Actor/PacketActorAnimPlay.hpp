#ifndef OPENMW_PACKETACTORANIMPLAY_HPP
#define OPENMW_PACKETACTORANIMPLAY_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorAnimPlay : public ActorPacket
    {
    public:
        PacketActorAnimPlay(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORANIMPLAY_HPP
