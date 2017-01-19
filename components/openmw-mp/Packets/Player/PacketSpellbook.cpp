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

    RW(player->packetSpells.action, send);

    if (!send)
        player->packetSpells.spells.clear();
    else
        player->packetSpells.count = (unsigned int) (player->packetSpells.spells.size());

    RW(player->packetSpells.count, send);

    for (unsigned int i = 0; i < player->packetSpells.count; i++)
    {
        ESM::Spell spell;

        if (send)
        {
            spell = player->packetSpells.spells[i];
            RW(spell.mId, send);
        }
        else
        {
            RW(spell.mId, send);
            player->packetSpells.spells.push_back(spell);
        }
    }

}
