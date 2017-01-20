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

    RW(player->spellbookChanges.action, send);

    if (!send)
        player->spellbookChanges.spells.clear();
    else
        player->spellbookChanges.count = (unsigned int) (player->spellbookChanges.spells.size());

    RW(player->spellbookChanges.count, send);

    for (unsigned int i = 0; i < player->spellbookChanges.count; i++)
    {
        ESM::Spell spell;

        if (send)
        {
            spell = player->spellbookChanges.spells[i];
            RW(spell.mId, send);
        }
        else
        {
            RW(spell.mId, send);
            player->spellbookChanges.spells.push_back(spell);
        }
    }

}
