#ifndef OPENMW_PACKETPLAYERMAP_HPP
#define OPENMW_PACKETPLAYERMAP_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerMap : public PlayerPacket
    {
    public:
        PacketPlayerMap(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERMAP_HPP
