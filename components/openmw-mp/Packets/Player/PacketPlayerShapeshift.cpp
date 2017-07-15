#include "PacketPlayerShapeshift.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerShapeshift::PacketPlayerShapeshift(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_SHAPESHIFT;
}

void PacketPlayerShapeshift::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->isWerewolf, send);
}
