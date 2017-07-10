#ifndef OPENMW_PACKETPLAYERJAIL_HPP
#define OPENMW_PACKETPLAYERJAIL_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerJail : public PlayerPacket
    {
    public:
        PacketPlayerJail(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERJAIL_HPP
