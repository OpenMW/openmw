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

    RW(player->getAttack()->attacker, send);
    RW(player->getAttack()->target, send);
    RW(player->getAttack()->refid, send);
    RW(player->getAttack()->type, send);
    RW(player->getAttack()->success, send);
    RW(player->getAttack()->damage, send);
    //
    RW(player->getAttack()->pressed, send);
    RW(player->getAttack()->knockdown, send);
    RW(player->getAttack()->block, send);
}
