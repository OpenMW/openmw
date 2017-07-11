#include "PacketPlayerResurrect.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>

using namespace mwmp;

PacketPlayerResurrect::PacketPlayerResurrect(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_RESURRECT;
}

void PacketPlayerResurrect::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->resurrectType, send);
}
