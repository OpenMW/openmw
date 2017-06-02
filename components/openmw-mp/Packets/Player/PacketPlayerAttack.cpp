//
// Created by koncord on 13.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAttack.hpp"

using namespace mwmp;

PacketPlayerAttack::PacketPlayerAttack(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ATTACK;
}

void PacketPlayerAttack::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->attack.target.refId, send, 1);
    RW(player->attack.target.refNumIndex, send);
    RW(player->attack.target.mpNum, send);
    RW(player->attack.target.guid, send);

    RW(player->attack.spellId, send, 1);
    RW(player->attack.type, send);
    RW(player->attack.success, send);
    RW(player->attack.damage, send, 0); // never compress damage

    RW(player->attack.pressed, send);
    RW(player->attack.knockdown, send);
    RW(player->attack.block, send);
}
