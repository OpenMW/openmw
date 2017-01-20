#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketJournal.hpp"

using namespace std;
using namespace mwmp;

PacketJournal::PacketJournal(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_JOURNAL;
}

void PacketJournal::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);
}
