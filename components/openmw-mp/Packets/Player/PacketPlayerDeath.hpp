//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETPLAYERDEATH_HPP
#define OPENMW_PACKETPLAYERDEATH_HPP


#include <components/openmw-mp/Packets/Player/PlayerPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketPlayerDeath: public PlayerPacket
    {
    public:
        PacketPlayerDeath(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
        {
            packetID = ID_PLAYER_DEATH;
        }
        void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
        {
            PlayerPacket::Packet(bs, player, send);
            RW(player->creatureStats.mDead, send);
        }
    };
}

#endif //OPENMW_PACKETPLAYERDEATH_HPP
