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

const char *StatsFunctions::GetName(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npc.mName.c_str();
}

const char *StatsFunctions::GetRace(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npc.mRace.c_str();
}

const char *StatsFunctions::GetHead(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npc.mHead.c_str();
}

const char *StatsFunctions::GetHairstyle(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npc.mHair.c_str();
}

int StatsFunctions::GetIsMale(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return player->npc.isMale();
}

const char *StatsFunctions::GetBirthsign(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->birthsign.c_str();
}

const char *StatsFunctions::GetCreatureModel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->creatureModel.c_str();
}

bool StatsFunctions::IsCreatureName(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->useCreatureName;
}

int StatsFunctions::GetLevel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->creatureStats.mLevel;
}

int StatsFunctions::GetLevelProgress(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npcStats.mLevelProgress;
}

double StatsFunctions::GetHealthBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->creatureStats.mDynamic[0].mBase;
}

double StatsFunctions::GetHealthCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->creatureStats.mDynamic[0].mCurrent;
}

double StatsFunctions::GetMagickaBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->creatureStats.mDynamic[1].mBase;
}

double StatsFunctions::GetMagickaCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->creatureStats.mDynamic[1].mCurrent;
}

double StatsFunctions::GetFatigueBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->creatureStats.mDynamic[2].mBase;
}

double StatsFunctions::GetFatigueCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->creatureStats.mDynamic[2].mCurrent;
}

int StatsFunctions::GetAttributeBase(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute >= Attribute::Length)
        return 0;

    return player->creatureStats.mAttributes[attribute].mBase;
}

int StatsFunctions::GetAttributeCurrent(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute >= Attribute::Length)
        return 0;

    return player->creatureStats.mAttributes[attribute].mCurrent;
}

int StatsFunctions::GetSkillBase(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->npcStats.mSkills[skill].mBase;
}

int StatsFunctions::GetSkillCurrent(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->npcStats.mSkills[skill].mCurrent;
}

double StatsFunctions::GetSkillProgress(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    if (skill >= Skill::Length)
        return 0;

    return player->npcStats.mSkills[skill].mProgress;
}

int StatsFunctions::GetSkillIncrease(unsigned short pid, unsigned int attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute > Attribute::Length)
        return 0;

    return player->npcStats.mSkillIncrease[attribute];
}

int StatsFunctions::GetBounty(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->npcStats.mBounty;
}

void StatsFunctions::SetName(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->npc.mName == name)
        return;

    player->npc.mName = name;
}

void StatsFunctions::SetRace(unsigned short pid, const char *race) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->npc.mRace == race)
        return;

    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Setting race for %s: %s -> %s", player->npc.mName.c_str(),
                       player->npc.mRace.c_str(), race);

    player->npc.mRace = race;
}

void StatsFunctions::SetHead(unsigned short pid, const char *head) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->npc.mHead == head)
        return;

    player->npc.mHead = head;
}

void StatsFunctions::SetHairstyle(unsigned short pid, const char *style) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->npc.mHair == style)
        return;

    player->npc.mHair = style;
}

void StatsFunctions::SetIsMale(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->npc.setIsMale(value == true);
}

void StatsFunctions::SetBirthsign(unsigned short pid, const char *sign) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    if (player->birthsign == sign)
        return;

    player->birthsign = sign;
}

void StatsFunctions::SetCreatureModel(unsigned short pid, const char *name, bool useCreatureName) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->creatureModel = name;
    player->useCreatureName = useCreatureName;

}

void StatsFunctions::SetLevel(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->creatureStats.mLevel = value;
}

void StatsFunctions::SetLevelProgress(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->npcStats.mLevelProgress = value;
}

void StatsFunctions::SetHealthBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->creatureStats.mDynamic[0].mBase = value;
}

void StatsFunctions::SetHealthCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->creatureStats.mDynamic[0].mCurrent = value;
}

void StatsFunctions::SetMagickaBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->creatureStats.mDynamic[1].mBase = value;
}

void StatsFunctions::SetMagickaCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->creatureStats.mDynamic[1].mCurrent = value;
}

void StatsFunctions::SetFatigueBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->creatureStats.mDynamic[2].mBase = value;
}

void StatsFunctions::SetFatigueCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->creatureStats.mDynamic[2].mCurrent = value;
}

void StatsFunctions::SetAttributeBase(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute >= Attribute::Length)
        return;

    player->creatureStats.mAttributes[attribute].mBase = value;
}

void StatsFunctions::SetAttributeCurrent(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute >= Attribute::Length)
        return;

    player->creatureStats.mAttributes[attribute].mCurrent = value;
}

void StatsFunctions::SetSkillBase(unsigned short pid, unsigned short skill, int value) noexcept  //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill >= Skill::Length)
        return;

    player->npcStats.mSkills[skill].mBase = value;
}

void StatsFunctions::SetSkillCurrent(unsigned short pid, unsigned short skill, int value) noexcept //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill >= Skill::Length)
        return;

    player->npcStats.mSkills[skill].mCurrent = value;
}

void StatsFunctions::SetSkillProgress(unsigned short pid, unsigned short skill, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    if (skill >= Skill::Length)
        return;

    player->npcStats.mSkills[skill].mProgress = value;
}

void StatsFunctions::SetSkillIncrease(unsigned short pid, unsigned int attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute > Attribute::Length)
        return;

    player->npcStats.mSkillIncrease[attribute] = value;
}

void StatsFunctions::SetBounty(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->npcStats.mBounty = value;
}

void StatsFunctions::SetCharGenStage(unsigned short pid, int start, int end) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->charGenStage.current = start;
    player->charGenStage.end = end;

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_CHARGEN)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_CHARGEN)->Send(false);
}

void StatsFunctions::Resurrect(unsigned short pid)
{
    Player *player;
    GET_PLAYER(pid, player,);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_RESURRECT)->RequestData(player->guid);
}

void StatsFunctions::SendBaseInfo(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BASEINFO)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BASEINFO)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BASEINFO)->Send(true);
}

void StatsFunctions::SendStatsDynamic(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_STATS_DYNAMIC)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_STATS_DYNAMIC)->Send(true);
}

void StatsFunctions::SendAttributes(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(true);
}

void StatsFunctions::SendSkills(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SKILL)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SKILL)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_SKILL)->Send(true);
}

void StatsFunctions::SendLevel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_LEVEL)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_LEVEL)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_LEVEL)->Send(true);
}

void StatsFunctions::SendBounty(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BOUNTY)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BOUNTY)->Send(false);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_BOUNTY)->Send(true);
}
