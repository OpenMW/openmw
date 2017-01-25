//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETRESURRECT_HPP
#define OPENMW_PACKETRESURRECT_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketResurrect: public PlayerPacket
    {
    public:
        PacketResurrect(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
        {
            packetID = ID_GAME_RESURRECT;
        }
        void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
        {
            PlayerPacket::Packet(bs, player, send);
            RW(player->creatureStats.mDead, send);
        }
    };
}


#endif //OPENMW_PACKETRESURRECT_HPP
