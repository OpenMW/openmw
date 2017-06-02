#ifndef OPENMW_PACKETACTORTEST_HPP
#define OPENMW_PACKETACTORTEST_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorTest : public ActorPacket
    {
    public:
        PacketActorTest(RakNet::RakPeerInterface *peer);

        virtual void Actor(BaseActor &actor, bool send);
    };
}

#endif //OPENMW_PACKETACTORTEST_HPP
