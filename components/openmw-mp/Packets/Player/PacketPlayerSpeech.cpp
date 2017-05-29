#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerSpeech.hpp"

mwmp::PacketPlayerSpeech::PacketPlayerSpeech(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_SPEECH;
}

void mwmp::PacketPlayerSpeech::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
