#include "PacketPlayerLevel.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerLevel::PacketPlayerLevel(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_LEVEL;
}

void PacketPlayerLevel::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    RW(player->creatureStats.mLevel, send);
}
