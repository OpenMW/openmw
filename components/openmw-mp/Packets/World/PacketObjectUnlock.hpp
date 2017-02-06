#ifndef OPENMW_PACKETOBJECTUNLOCK_HPP
#define OPENMW_PACKETOBJECTUNLOCK_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketObjectUnlock : public WorldPacket
    {
    public:
        PacketObjectUnlock(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BaseEvent *event, bool send);
    };
}

#endif //OPENMW_PACKETOBJECTUNLOCK_HPP
