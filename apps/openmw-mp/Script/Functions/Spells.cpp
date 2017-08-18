#include "Spells.hpp"

#include <components/misc/stringops.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

using namespace mwmp;

void SpellFunctions::InitializeSpellbookChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->spellbookChanges.spells.clear();
}

unsigned int SpellFunctions::GetSpellbookChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->spellbookChanges.count;
}

unsigned int SpellFunctions::GetSpellbookChangesAction(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->spellbookChanges.action;
}

void SpellFunctions::SetSpellbookChangesAction(unsigned short pid, unsigned char action) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->spellbookChanges.action = action;
}

void SpellFunctions::AddSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

    player->spellbookChanges.spells.push_back(spell);
}

const char *SpellFunctions::GetSpellId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->spellbookChanges.count)
        return "invalid";

    return player->spellbookChanges.spells.at(i).mId.c_str();
}

void SpellFunctions::SendSpellbookChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SPELLBOOK)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SPELLBOOK)->Send(toOthers);
}
