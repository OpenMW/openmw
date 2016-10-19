//
// Created by koncord on 28.04.16.
//

#ifndef OPENMW_PACKETHANDSHAKE_HPP
#define OPENMW_PACKETHANDSHAKE_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketHandshake : public PlayerPacket
    {
    public:
        PacketHandshake(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send);
    };
}

#endif //OPENMW_PACKETHANDSHAKE_HPP
