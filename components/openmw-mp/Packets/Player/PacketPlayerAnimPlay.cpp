#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerAnimPlay.hpp"

mwmp::PacketPlayerAnimPlay::PacketPlayerAnimPlay(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ANIM_PLAY;
}

void mwmp::PacketPlayerAnimPlay::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
