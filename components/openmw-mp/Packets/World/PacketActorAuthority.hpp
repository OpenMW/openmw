#ifndef OPENMW_PACKETACTORAUTHORITY_HPP
#define OPENMW_PACKETACTORAUTHORITY_HPP

#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

namespace mwmp
{
    class PacketActorAuthority : public WorldPacket
    {
    public:
        PacketActorAuthority(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETACTORAUTHORITY_HPP
