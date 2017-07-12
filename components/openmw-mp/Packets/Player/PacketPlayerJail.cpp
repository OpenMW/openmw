#include "PacketPlayerJail.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>

using namespace mwmp;

PacketPlayerJail::PacketPlayerJail(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_JAIL;
}

void PacketPlayerJail::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->jailDays, send);
    RW(player->ignoreJailTeleportation, send);
    RW(player->jailText, send, 1);
}
