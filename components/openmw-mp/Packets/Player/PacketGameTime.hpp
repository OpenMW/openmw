//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_PACKETGAMETIME_HPP
#define OPENMW_PACKETGAMETIME_HPP

#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>

namespace mwmp
{
    class PacketGameTime : public PlayerPacket
    {
    public:
        PacketGameTime(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *bs, bool send);
    };
}

#endif //OPENMW_PACKETGAMETIME_HPP
