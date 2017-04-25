#ifndef OPENMW_PACKETPLAYERBOUNTY_HPP
#define OPENMW_PACKETPLAYERBOUNTY_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerBounty : public PlayerPacket
    {
    public:
        PacketPlayerBounty(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERBOUNTY_HPP
