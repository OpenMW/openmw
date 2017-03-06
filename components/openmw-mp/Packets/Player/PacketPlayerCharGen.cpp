//
// Created by koncord on 08.03.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCharGen.hpp"

mwmp::PacketPlayerCharGen::PacketPlayerCharGen(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CHARGEN;
}

void mwmp::PacketPlayerCharGen::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->charGenStage, send);

}
