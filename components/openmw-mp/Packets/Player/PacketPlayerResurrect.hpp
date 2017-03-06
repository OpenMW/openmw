//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETPLAYERRESURRECT_HPP
#define OPENMW_PACKETPLAYERRESURRECT_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketPlayerResurrect: public PlayerPacket
    {
    public:
        PacketPlayerResurrect(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
        {
            packetID = ID_PLAYER_RESURRECT;
        }
        void Packet(RakNet::BitStream *bs, bool send)
        {
            PlayerPacket::Packet(bs, send);
            RW(player->creatureStats.mDead, send);
        }
    };
}


#endif //OPENMW_PACKETPLAYERRESURRECT_HPP
