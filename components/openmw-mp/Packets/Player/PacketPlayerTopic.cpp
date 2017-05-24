#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerTopic.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerTopic::PacketPlayerTopic(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_TOPIC;
}

void PacketPlayerTopic::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);
}
