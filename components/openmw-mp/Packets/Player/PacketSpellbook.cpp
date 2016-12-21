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

    RW(player->spellbook.action, send);

    if (!send)
        player->spellbook.spells.clear();
    else
        player->spellbook.count = (unsigned int) (player->spellbook.spells.size());

    RW(player->spellbook.count, send);

    for (unsigned int i = 0; i < player->spellbook.count; i++)
    {
        Spell spell;

        if (send)
        {
            spell = player->spellbook.spells[i];
            RW(spell.id, send);
        }
        else
        {
            RW(spell.id, send);
            player->spellbook.spells.push_back(spell);
        }
    }

}
