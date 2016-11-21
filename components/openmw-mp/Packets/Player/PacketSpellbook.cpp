#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketSpellbook.hpp"

using namespace std;
using namespace mwmp;

PacketSpellbook::PacketSpellbook(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_SPELLBOOK;
}

void PacketSpellbook::Packet(RakNet::BitStream *bs, BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);
}
