#ifndef OPENMW_PACKETPLAYERREGIONAUTHORITY_HPP
#define OPENMW_PACKETPLAYERREGIONAUTHORITY_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerRegionAuthority : public PlayerPacket
    {
    public:
        PacketPlayerRegionAuthority(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERREGIONAUTHORITY_HPP
