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

    RW(player->GetAttack()->attacker, send);
    RW(player->GetAttack()->target, send);
    RW(player->GetAttack()->refid, send);
    RW(player->GetAttack()->type, send);
    RW(player->GetAttack()->success, send);
    RW(player->GetAttack()->damage, send);
    //
    RW(player->GetAttack()->pressed, send);
    RW(player->GetAttack()->knockdown, send);
    RW(player->GetAttack()->block, send);
}
