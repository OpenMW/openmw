#ifndef OPENMW_PACKETACTORLIST_HPP
#define OPENMW_PACKETACTORLIST_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketActorList : public WorldPacket
    {
    public:
        PacketActorList(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORLIST_HPP
