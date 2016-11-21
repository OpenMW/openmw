#include "Spells.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/misc/stringops.hpp>

using namespace mwmp;

unsigned int SpellFunctions::GetSpellbookSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->spellbook.count;
}

void SpellFunctions::AddSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    Spell spell;
    spell.id = spellId;

    player->spellbookSendBuffer.spells.push_back(spell);
    player->spellbookSendBuffer.action = Spellbook::ADD;
}

void SpellFunctions::RemoveSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    Spell spell;
    spell.id = spellId;

    player->spellbookSendBuffer.spells.push_back(spell);
    player->spellbookSendBuffer.action = Spellbook::REMOVE;
}

void SpellFunctions::ClearSpellbook(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->spellbookSendBuffer.spells.clear();
    player->spellbookSendBuffer.action = Spellbook::UPDATE;
}

bool SpellFunctions::HasSpell(unsigned short pid, const char* spellId)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for (int i = 0; i < player->spellbook.count; i++)
        if (Misc::StringUtils::ciEqual(player->spellbook.spells.at(i).id, spellId))
            return true;
    return false;
}

const char *SpellFunctions::GetSpellId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->spellbook.count)
        return "invalid";

    return player->spellbook.spells.at(i).id.c_str();
}

void SpellFunctions::SendSpellbook(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );
    std::swap(player->spellbook, player->spellbookSendBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_SPELLBOOK)->Send(player, false);
    player->spellbook = std::move(player->spellbookSendBuffer);
    player->spellbookSendBuffer.spells.clear();
}
