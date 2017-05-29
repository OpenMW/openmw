#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerMap.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerMap::PacketPlayerMap(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_MAP;
}

void PacketPlayerMap::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
