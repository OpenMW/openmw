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
    if(send)
    {
        rot[0] = player->position.rot[0] * 0.1f;
        rot[1] = player->position.rot[2] * 0.1f;
    }
    RW(rot[0], send, 1);
    RW(rot[1], send, 1);
    if(!send)
    {
        player->position.rot[0] = rot[0] / 0.1f;
        player->position.rot[2] = rot[1] / 0.1f;
    }

    RW(player->position.pos, send, 1);
    RW(player->direction.pos, send, 1);
}
