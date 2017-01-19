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

    return player->spellbook.size();
}

void SpellFunctions::AddSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

    player->packetSpellsBuffer.spells.push_back(spell);
    player->packetSpellsBuffer.action = PacketSpells::ADD;
}

void SpellFunctions::RemoveSpell(unsigned short pid, const char* spellId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mId = spellId;

    player->packetSpellsBuffer.spells.push_back(spell);
    player->packetSpellsBuffer.action = PacketSpells::REMOVE;
}

void SpellFunctions::ClearSpellbook(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->packetSpellsBuffer.spells.clear();
    player->packetSpellsBuffer.action = PacketSpells::UPDATE;
}

bool SpellFunctions::HasSpell(unsigned short pid, const char* spellId)
{
    Player *player;
    GET_PLAYER(pid, player, false);

    for (unsigned int i = 0; i < player->spellbook.size(); i++)
        if (Misc::StringUtils::ciEqual(player->spellbook.at(i).mId, spellId))
            return true;
    return false;
}

const char *SpellFunctions::GetSpellId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->spellbook.size())
        return "invalid";

    return player->spellbook.at(i).mId.c_str();
}

void SpellFunctions::SendSpells(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    for (auto spell : player->packetSpellsBuffer.spells)
    {
        if (player->packetSpellsBuffer.action == PacketSpells::ADD)
            player->spellbook.push_back(spell);
        else if (player->packetSpells.action == PacketSpells::REMOVE)
            player->spellbook.erase(remove_if(player->spellbook.begin(), player->spellbook.end(), [&spell](ESM::Spell s)->bool
            {return spell.mId == s.mId; }), player->spellbook.end());
    }

    std::swap(player->packetSpells, player->packetSpellsBuffer);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_SPELLBOOK)->Send(player, false);
    player->packetSpells = std::move(player->packetSpellsBuffer);
    player->packetSpellsBuffer.spells.clear();
}
