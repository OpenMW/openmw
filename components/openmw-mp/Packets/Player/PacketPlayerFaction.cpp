#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerFaction.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerFaction::PacketPlayerFaction(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_FACTION;
}

void PacketPlayerFaction::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);
}
