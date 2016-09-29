//
// Created by koncord on 29.02.16.
//
#include "Stats.hpp"

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/esm/attr.hpp>
#include <components/esm/loadskil.hpp>
#include <components/misc/stringops.hpp>
#include <components/openmw-mp/Log.hpp>
#include <iostream>

using namespace std;
using namespace ESM;

void StatsFunctions::SetName(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mName == name)
        return;

    player->Npc()->mName = name;
}

const char *StatsFunctions::GetName(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mName.c_str();
}

void StatsFunctions::SetBirthsign(unsigned short pid, const char *sign) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (*player->BirthSign() == sign)
        return;

    *player->BirthSign() = sign;
}

const char *StatsFunctions::GetBirthsign(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);


    return player->BirthSign()->c_str();
}

void StatsFunctions::SetRace(unsigned short pid, const char *race) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mRace == race)
        return;

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Setting race for %s: %s -> %s",
        player->Npc()->mName.c_str(),
        player->Npc()->mRace.c_str(),
        race);

    player->Npc()->mRace = race;
}

const char *StatsFunctions::GetRace(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mRace.c_str();
}

void StatsFunctions::SetHead(unsigned short pid, const char *head) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mHead == head)
        return;

    player->Npc()->mHead = head;
}

const char *StatsFunctions::GetHead(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mHead.c_str();
}

void StatsFunctions::SetHairstyle(unsigned short pid, const char *style) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mHair == style)
        return;

    player->Npc()->mHair = style;
}

const char *StatsFunctions::GetHairstyle(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mHair.c_str();
}

int StatsFunctions::GetIsMale(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,false);

    return player->Npc()->isMale();
}

void StatsFunctions::SetIsMale(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->Npc()->setIsMale(value == true);
}

int StatsFunctions::GetLevel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mLevel;
}

void StatsFunctions::SetLevel(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->CreatureStats()->mLevel = value;
}

double StatsFunctions::GetHealthBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[0].mBase;
}

void StatsFunctions::SetHealthBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[0].mBase = value;
}

double StatsFunctions::GetHealthCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[0].mCurrent;
}

void StatsFunctions::SetHealthCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[0].mCurrent = value;
}

double StatsFunctions::GetMagickaBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[1].mBase;
}

void StatsFunctions::SetMagickaBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[1].mBase = value;
}

double StatsFunctions::GetMagickaCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[1].mCurrent;
}

void StatsFunctions::SetMagickaCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[1].mCurrent = value;
}

double StatsFunctions::GetFatigueBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[2].mBase;
}

void StatsFunctions::SetFatigueBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[2].mBase = value;
}

double StatsFunctions::GetFatigueCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[2].mCurrent;
}

void StatsFunctions::SetFatigueCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[2].mCurrent = value;
}

int StatsFunctions::GetAttributeCount() noexcept
{
    return Attribute::Length;
}

int StatsFunctions::GetSkillCount() noexcept
{
    return Skill::Length;
}

int StatsFunctions::GetAttributeId(const char *name) noexcept
{
    for (int x = 0; x < Attribute::Length; x++)
    {
        if (Misc::StringUtils::ciEqual(name, Attribute::sAttributeNames[x]))
        {
            return x;
        }
    }

    return -1;
}

int StatsFunctions::GetSkillId(const char *name) noexcept
{
    for (int x = 0; x < Skill::Length; x++)
    {
        if (Misc::StringUtils::ciEqual(name, Skill::sSkillNames[x]))
        {
            return x;
        }
    }

    return -1;
}

const char *StatsFunctions::GetAttributeName(unsigned short attribute) noexcept
{
    if (attribute >= Attribute::Length)
        return "invalid";

    return Attribute::sAttributeNames[attribute].c_str();
}

const char *StatsFunctions::GetSkillName(unsigned short skill) noexcept
{
    if (skill >= Skill::Length)
        return "invalid";

    return Skill::sSkillNames[skill].c_str();
}

int StatsFunctions::GetAttributeBase(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute >= Attribute::Length)
        return 0;

    return player->CreatureStats()->mAttributes[attribute].mBase;
}

void StatsFunctions::SetAttributeBase(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute >= Attribute::Length)
        return;

    player->CreatureStats()->mAttributes[attribute].mBase = value;
}

int StatsFunctions::GetAttributeCurrent(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute >= Attribute::Length)
        return 0;

    return player->CreatureStats()->mAttributes[attribute].mCurrent;
}

void StatsFunctions::SetAttributeCurrent(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute >= Attribute::Length)
        return;

    player->CreatureStats()->mAttributes[attribute].mCurrent = value;
}

int StatsFunctions::GetSkillBase(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->NpcStats()->mSkills[skill].mBase;
}

void StatsFunctions::SetSkillBase(unsigned short pid, unsigned short skill, int value) noexcept  //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill >= Skill::Length)
        return;

    player->NpcStats()->mSkills[skill].mBase = value;
}

int StatsFunctions::GetSkillCurrent(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->NpcStats()->mSkills[skill].mCurrent;
}

void StatsFunctions::SetSkillCurrent(unsigned short pid, unsigned short skill, int value) noexcept //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill >= Skill::Length)
        return;

    player->NpcStats()->mSkills[skill].mCurrent = value;
}

int StatsFunctions::GetSkillProgress(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->NpcStats()->mSkills[skill].mProgress;
}

void StatsFunctions::SetSkillProgress(unsigned short pid, unsigned short skill, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    if (skill >= Skill::Length)
        return;

    player->NpcStats()->mSkills[skill].mProgress = value;
}

int StatsFunctions::GetSkillIncrease(unsigned short pid, unsigned int pos) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (pos > 7)
        return 0;

    return player->NpcStats()->mSkillIncrease[pos];
}

void StatsFunctions::SetSkillIncrease(unsigned short pid, unsigned int pos, int value) noexcept // TODO: need packet for transmit it
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (pos > 7)
        return;

    player->NpcStats()->mSkillIncrease[pos] = value;
}

void StatsFunctions::SetCharGenStage(unsigned short pid, int start, int end) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CharGenStage()->current = start;
    player->CharGenStage()->end = end;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_CHARGEN)->Send(player, false);
}

void StatsFunctions::Resurrect(unsigned short pid)
{
    Player *player;
    GET_PLAYER(pid, player,);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_RESURRECT)->RequestData(player->guid);
}

void StatsFunctions::SendBaseInfo(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_BASE_INFO)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_BASE_INFO)->Send(player, true);
}

void StatsFunctions::SendDynamicStats(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_DYNAMICSTATS)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_DYNAMICSTATS)->Send(player, true);
}

void StatsFunctions::SendAttributes(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_ATTRIBUTE)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_ATTRIBUTE)->Send(player, true);
}

void StatsFunctions::SendSkills(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_SKILL)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_SKILL)->Send(player, true);
}

void StatsFunctions::SendLevel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_LEVEL)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_LEVEL)->Send(player, true);
}
