#include "Spells.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

unsigned int SpellFunctions::GetSpellbookChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->spellbookChanges.count;
}

unsigned int SpellFunctions::GetSpellbookAction(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->spellbookChanges.action;
}

void SpellFunctions::AddSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

    player->spellbookChangesBuffer.spells.push_back(spell);
    player->spellbookChangesBuffer.action = SpellbookChanges::ADD;
}

void SpellFunctions::RemoveSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

    player->spellbookChangesBuffer.spells.push_back(spell);
    player->spellbookChangesBuffer.action = SpellbookChanges::REMOVE;
}

void SpellFunctions::ClearSpellbook(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->spellbookChangesBuffer.spells.clear();
    player->spellbookChangesBuffer.action = SpellbookChanges::SET;
}

const char *SpellFunctions::GetSpellId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->spellbookChanges.count)
        return "invalid";

    return player->spellbookChanges.spells.at(i).mId.c_str();
}

void SpellFunctions::SendSpellbookChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    std::swap(player->spellbookChanges, player->spellbookChangesBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_PLAYER_SPELLBOOK)->Send(player, false);
    player->spellbookChanges = std::move(player->spellbookChangesBuffer);
    player->spellbookChangesBuffer.spells.clear();
}
