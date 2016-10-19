//
// Created by koncord on 05.01.16.
//

#ifndef OPENMW_PACKETPOSITION_HPP
#define OPENMW_PACKETPOSITION_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketPosition : public PlayerPacket
    {
    public:
        PacketPosition(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETPOSITION_HPP
