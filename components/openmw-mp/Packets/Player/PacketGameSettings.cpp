#include "PacketGameSettings.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketGameSettings::PacketGameSettings(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_SETTINGS;
    orderChannel = CHANNEL_SYSTEM;
}

void PacketGameSettings::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->consoleAllowed, send);
    RW(player->difficulty, send);
}
