#ifndef OPENMW_PACKETPLAYERREGIONCHANGE_HPP
#define OPENMW_PACKETPLAYERREGIONCHANGE_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerKillCount : public PlayerPacket
    {
    public:
        PacketPlayerKillCount(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERREGIONCHANGE_HPP
