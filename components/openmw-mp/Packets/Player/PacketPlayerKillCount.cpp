#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerKillCount.hpp"

mwmp::PacketPlayerKillCount::PacketPlayerKillCount(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_KILL_COUNT;
}

void mwmp::PacketPlayerKillCount::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
