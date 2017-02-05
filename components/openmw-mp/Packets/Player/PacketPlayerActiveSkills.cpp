//
// Created by koncord on 03.12.16.
//

#include "PacketPlayerActiveSkills.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketPlayerActiveSkills::PacketPlayerActiveSkills(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_PLAYER_ACTIVESKILLS;
}

void PacketPlayerActiveSkills::Packet(RakNet::BitStream *bs, mwmp::BasePlayer *player, bool send)
{
    PlayerPacket::Packet(bs, player, send);

    unsigned long spells = 0;

    if (send)
        spells = player->activeSpells.mSpells.size();

    RW(spells, send);

    if (send)
        for (ESM::ActiveSpells::TContainer::const_iterator spell = player->activeSpells.mSpells.begin();
             spell != player->activeSpells.mSpells.end(); ++spell)
        {
            RW(spell->first, true);
            RW(spell->second.mTimeStamp, true);
            unsigned long effects = spell->second.mEffects.size();
            RW(effects, true);

            for (std::vector<ESM::ActiveEffect>::const_iterator effect = spell->second.mEffects.begin();
                 effect != spell->second.mEffects.end(); ++effect)
                RW(effect, true);

        }
    else
        for (unsigned int i = 0; i < spells; i++)
        {
            ESM::ActiveSpells::TContainer::value_type spell;

            RW(spell.first, false);
            RW(spell.second.mTimeStamp, false);
            unsigned long effects;
            RW(effects, false);

            ESM::ActiveEffect effect;
            for (unsigned int j = 0; j < effects; j++)
            {
                RW(effect, false);
                spell.second.mEffects.push_back(effect);
            }
            player->activeSpells.mSpells.insert(spell);
        }
}
