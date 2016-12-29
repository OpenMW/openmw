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

    return player->realSpellbook.size();
}

void SpellFunctions::AddSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

    player->spellbookSendBuffer.spells.push_back(spell);
    player->spellbookSendBuffer.action = Spellbook::ADD;
}

void SpellFunctions::RemoveSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

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

    for (unsigned int i = 0; i < player->realSpellbook.size(); i++)
        if (Misc::StringUtils::ciEqual(player->realSpellbook.at(i).mId, spellId))
            return true;
    return false;
}

const char *SpellFunctions::GetSpellId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->realSpellbook.size())
        return "invalid";

    return player->realSpellbook.at(i).mId.c_str();
}

void SpellFunctions::SendSpellbook(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    for (auto spell : player->spellbookSendBuffer.spells)
    {
        if (player->spellbookSendBuffer.action == Spellbook::ADD)
            player->realSpellbook.push_back(spell);
        else if (player->spellbook.action == Spellbook::REMOVE)
            player->realSpellbook.erase(remove_if(player->realSpellbook.begin(), player->realSpellbook.end(), [&spell](ESM::Spell s)->bool
            {return spell.mId == s.mId; }), player->realSpellbook.end());
    }

    std::swap(player->spellbook, player->spellbookSendBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_SPELLBOOK)->Send(player, false);
    player->spellbook = std::move(player->spellbookSendBuffer);
    player->spellbookSendBuffer.spells.clear();
}
