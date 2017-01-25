#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerCellLoad.hpp"


mwmp::PacketPlayerCellLoad::PacketPlayerCellLoad(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_CELL_CHANGE;
    priority = IMMEDIATE_PRIORITY;
    reliability = RELIABLE_ORDERED;
}

void mwmp::PacketPlayerCellLoad::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->getCell()->mData.mFlags, send);
    RW(player->getCell()->mData.mX, send);
    RW(player->getCell()->mData.mY, send);
    RW(player->getCell()->mName, send);
}
