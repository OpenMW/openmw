//
// Created by koncord on 05.01.16.
//

#ifndef OPENMW_PACKETPLAYERPOSITION_HPP
#define OPENMW_PACKETPLAYERPOSITION_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPlayerPosition : public PlayerPacket
    {
    public:
        PacketPlayerPosition(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETPLAYERPOSITION_HPP
