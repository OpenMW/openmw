//
// Created by koncord on 08.03.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketCharGen.hpp"

mwmp::PacketCharGen::PacketCharGen(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_CHARGEN;
}

void mwmp::PacketCharGen::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(*player->CharGenStage(), send);

}
