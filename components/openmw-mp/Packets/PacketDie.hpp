//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETDIE_HPP
#define OPENMW_PACKETDIE_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketDie: public BasePacket
    {
    public:
        PacketDie(RakNet::RakPeerInterface *peer) : BasePacket(peer)
        {
            packetID = ID_GAME_DIE;
        }
        void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
        {
            BasePacket::Packet(bs, player, send);
            RW(player->CreatureStats()->mDead, send);
        }
    };
}

#endif //OPENMW_PACKETDIE_HPP
