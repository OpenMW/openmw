//
// Created by koncord on 05.01.16.
//

#include "PacketPlayerPosition.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace std;
using namespace mwmp;

PacketPlayerPosition::PacketPlayerPosition(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_POSITION;
    priority = MEDIUM_PRIORITY;
    //reliability = UNRELIABLE_SEQUENCED;
}

void PacketPlayerPosition::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    float rot[2];
    unsigned char dir;
    if (send)
    {
        rot[0] = player->position.rot[0] * 0.1f;
        rot[1] = player->position.rot[2] * 0.1f;

        dir = (player->direction.pos[0] >= 0 ?  (unsigned char)(player->direction.pos[0]) : (unsigned char) 0x3) << 2; // pack direction
        dir += (player->direction.pos[1] >= 0 ?  (unsigned char)(player->direction.pos[1]) : (unsigned char) 0x3);
    }
    RW(rot[0], send, 1);
    RW(rot[1], send, 1);

    RW(player->position.pos, send, 1);
    RW(dir, send);

    if (!send)
    {
        player->position.rot[0] = rot[0] / 0.1f;
        player->position.rot[2] = rot[1] / 0.1f;
        player->direction.pos[0] = (dir >> 2) == 0x3 ? -1 : dir >> 2; // unpack direction
        player->direction.pos[1] = (dir & 0x3) == 0x3 ? -1 : dir & 0x3;
    }
}
