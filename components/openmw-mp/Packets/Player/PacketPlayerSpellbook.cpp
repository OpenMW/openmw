#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketPlayerSpellbook.hpp"

using namespace std;
using namespace mwmp;

PacketPlayerSpellbook::PacketPlayerSpellbook(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_SPELLBOOK;
}

void PacketPlayerSpellbook::Packet(RakNet::BitStream *bs, bool send)
{
    PlayerPacket::Packet(bs, send);

    RW(player->spellbookChanges.action, send);

    if (send)
        player->spellbookChanges.count = (unsigned int) (player->spellbookChanges.spells.size());
    else
        player->spellbookChanges.spells.clear();

    RW(player->spellbookChanges.count, send);

    for (unsigned int i = 0; i < player->spellbookChanges.count; i++)
    {
        ESM::Spell spell;

        if (send)
            spell = player->spellbookChanges.spells.at(i);

        RW(spell.mId, send, 1);

        if(spell.mId.find("$dynamic") != string::npos)
        {
            RW(spell.mName, send, 1);

            RW(spell.mData.mType, send, 1);
            RW(spell.mData.mCost, send, 1);
            RW(spell.mData.mFlags, send, 1);

            int effectCount = 0;
            if (send)
                effectCount = spell.mEffects.mList.size();

            RW(effectCount, send, 1);

            for (unsigned int j = 0; j < effectCount; j++)
            {
                ESM::ENAMstruct effect;
                if (send)
                    effect = spell.mEffects.mList.at(j);

                RW(effect.mEffectID, send, 1);
                RW(effect.mSkill, send, 1);
                RW(effect.mAttribute, send, 1);
                RW(effect.mRange, send, 1);
                RW(effect.mArea, send, 1);
                RW(effect.mDuration, send, 1);
                RW(effect.mMagnMin, send, 1);
                RW(effect.mMagnMax, send, 1);

                if(!send)
                    spell.mEffects.mList.push_back(effect);
            }
        }

        if (!send)
            player->spellbookChanges.spells.push_back(spell);
    }

}
