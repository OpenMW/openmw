#ifndef OPENMW_PACKETPLAYERRESURRECT_HPP
#define OPENMW_PACKETPLAYERRESURRECT_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerResurrect : public PlayerPacket
    {
    public:
        PacketPlayerResurrect(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERRESURRECT_HPP
