#include "PacketPlayerBounty.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerBounty::PacketPlayerBounty(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_BOUNTY;
}

void PacketPlayerBounty::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->npcStats.mBounty, send);
}
