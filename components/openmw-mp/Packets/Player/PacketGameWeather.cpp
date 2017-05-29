#include "PacketGameWeather.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketGameWeather::PacketGameWeather(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_WEATHER;
    orderChannel = CHANNEL_SYSTEM;
}

void PacketGameWeather::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
