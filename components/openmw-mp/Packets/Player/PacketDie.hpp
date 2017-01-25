//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETDIE_HPP
#define OPENMW_PACKETDIE_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketDie: public PlayerPacket
    {
    public:
        PacketDie(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
        {
            packetID = ID_GAME_DIE;
        }
        void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
        {
            PlayerPacket::Packet(bs, player, send);
            RW(player->creatureStats.mDead, send);
        }
    };
}

#endif //OPENMW_PACKETDIE_HPP
