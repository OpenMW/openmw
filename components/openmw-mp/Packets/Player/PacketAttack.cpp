//
// Created by koncord on 13.01.16.
//

#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketAttack.hpp"

using namespace mwmp;

PacketAttack::PacketAttack(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_ATTACK;
}

void PacketAttack::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->attack.attacker, send);
    RW(player->attack.target, send);
    RW(player->attack.refid, send);
    RW(player->attack.type, send);
    RW(player->attack.success, send);
    RW(player->attack.damage, send);
    //
    RW(player->attack.pressed, send);
    RW(player->attack.knockdown, send);
    RW(player->attack.block, send);
}
