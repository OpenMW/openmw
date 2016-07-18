//
// Created by koncord on 13.01.16.
//

#ifndef OPENMW_PACKETRESURRECT_HPP
#define OPENMW_PACKETRESURRECT_HPP


#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketResurrect: public BasePacket
    {
    public:
        PacketResurrect(RakNet::RakPeerInterface *peer) : BasePacket(peer)
        {
            packetID = ID_GAME_RESURRECT;
        }
        void Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
        {
            BasePacket::Packet(bs, player, send);
            RW(player->CreatureStats()->mDead, send);
        }
    };
}


#endif //OPENMW_PACKETRESURRECT_HPP
