#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerBook.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerBook::PacketPlayerBook(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_BOOK;
}

void PacketPlayerBook::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    // Placeholder to be filled in later
}
