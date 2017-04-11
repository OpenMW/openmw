#ifndef OPENMW_PACKETACTORDRAWSTATE_HPP
#define OPENMW_PACKETACTORDRAWSTATE_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorDrawState : public ActorPacket
    {
    public:
        PacketActorDrawState(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORDRAWSTATE_HPP
