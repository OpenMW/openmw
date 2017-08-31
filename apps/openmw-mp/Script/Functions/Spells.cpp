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

void SpellFunctions::AddCustomSpell(unsigned short pid, const char* spellId, const char* spellName) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Spell spell;
    spell.mName = spellName;
    spell.mId = spellId;

    player->spellbookChanges.spells.push_back(spell);
}

void SpellFunctions::AddCustomSpellData(unsigned short pid, const char* spellId, int type, int cost, int flags) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    int index = -1;
    for(int i = 0; i < player->spellbookChanges.spells.size(); i++)
    {
        if( strcmp(player->spellbookChanges.spells.at(i).mId.c_str(), spellId) == 0)
        {
            index = i;
        }
    }

    if(index == -1)
        return;

    player->spellbookChanges.spells.at(index).mData.mType = type;
    player->spellbookChanges.spells.at(index).mData.mCost = cost;
    player->spellbookChanges.spells.at(index).mData.mFlags = flags;
}

void SpellFunctions::AddCustomSpellEffect(unsigned short pid, const char* spellId, short effectId, signed char mSkill, signed char mAttribute, int mRange, int mArea, int mDuration, int mMagnMin, int mMagnMax) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    int index = -1;
    for(int i = 0; i < player->spellbookChanges.spells.size(); i++)
    {
        if( strcmp(player->spellbookChanges.spells.at(i).mId.c_str(), spellId) == 0)
        {
            index = i;
        }
    }

    if(index == -1)
        return;

    ESM::ENAMstruct effect;
    effect.mEffectID = effectId;
    effect.mSkill = mSkill;
    effect.mAttribute = mAttribute;
    effect.mRange = mRange;
    effect.mArea = mArea;
    effect.mDuration = mDuration;
    effect.mMagnMin = mMagnMin;
    effect.mMagnMax = mMagnMax;

    player->spellbookChanges.spells.at(index).mEffects.mList.push_back(effect);
}

const char *SpellFunctions::GetSpellId(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->spellbookChanges.count)
        return "invalid";

    return player->spellbookChanges.spells.at(i).mId.c_str();
}

const char *SpellFunctions::GetSpellName(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->spellbookChanges.count)
        return "invalid";

    return player->spellbookChanges.spells.at(i).mName.c_str();
}

int SpellFunctions::GetSpellType(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mData.mType;
}

int SpellFunctions::GetSpellCost(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mData.mCost;
}

int SpellFunctions::GetSpellFlags(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mData.mFlags;
}

unsigned int SpellFunctions::GetSpellEffectCount(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.size();
}

short SpellFunctions::GetSpellEffectId(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mEffectID;
}

signed char SpellFunctions::GetSpellEffectSkill(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mSkill;
}

signed char SpellFunctions::GetSpellEffectAttribute(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mAttribute;
}

int SpellFunctions::GetSpellEffectRange(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mRange;
}

int SpellFunctions::GetSpellEffectArea(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mArea;
}

int SpellFunctions::GetSpellEffectDuration(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mDuration;
}

int SpellFunctions::GetSpellEffectMagnMin(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mMagnMin;
}

int SpellFunctions::GetSpellEffectMagnMax(unsigned short pid, unsigned int i, unsigned int j) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (i >= player->spellbookChanges.count)
        return 0;

    return player->spellbookChanges.spells.at(i).mEffects.mList.at(j).mMagnMax;
}

void SpellFunctions::SendSpellbookChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SPELLBOOK)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SPELLBOOK)->Send(toOthers);
}
