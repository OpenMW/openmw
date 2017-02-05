//
// Created by koncord on 07.01.16.
//

#ifndef OPENMW_PACKETPLAYERBASEINFO_HPP
#define OPENMW_PACKETPLAYERBASEINFO_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerBaseInfo : public PlayerPacket
    {
    public:
        PacketPlayerBaseInfo(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERBASEINFO_HPP
