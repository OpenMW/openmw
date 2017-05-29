#ifndef OPENMW_PACKETACTORAI_HPP
#define OPENMW_PACKETACTORAI_HPP

#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>

namespace mwmp
{
    class PacketActorAI : public ActorPacket
    {
    public:
        PacketActorAI(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORAI_HPP
