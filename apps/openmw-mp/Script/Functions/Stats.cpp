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

void StatsFunctions::setName(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mName == name)
        return;

    player->Npc()->mName = name;
}

const char *StatsFunctions::getName(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mName.c_str();
}

void StatsFunctions::setBirthsign(unsigned short pid, const char *sign) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (*player->BirthSign() == sign)
        return;

    *player->BirthSign() = sign;
}

const char *StatsFunctions::getBirthsign(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);


    return player->BirthSign()->c_str();
}

void StatsFunctions::setRace(unsigned short pid, const char *race) noexcept
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

const char *StatsFunctions::getRace(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mRace.c_str();
}

void StatsFunctions::setHead(unsigned short pid, const char *head) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mHead == head)
        return;

    player->Npc()->mHead = head;
}

const char *StatsFunctions::getHead(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mHead.c_str();
}

void StatsFunctions::setHairstyle(unsigned short pid, const char *style) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mHair == style)
        return;

    player->Npc()->mHair = style;
}

const char *StatsFunctions::getHairstyle(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mHair.c_str();
}

int StatsFunctions::getIsMale(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,false);

    return player->Npc()->isMale();
}

void StatsFunctions::setIsMale(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->Npc()->setIsMale(value == true);
}

int StatsFunctions::getLevel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mLevel;
}

void StatsFunctions::setLevel(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->CreatureStats()->mLevel = value;
}

int StatsFunctions::getLevelProgress(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->NpcStats()->mLevelProgress;
}

void StatsFunctions::setLevelProgress(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->NpcStats()->mLevelProgress = value;
}

double StatsFunctions::getHealthBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[0].mBase;
}

void StatsFunctions::setHealthBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[0].mBase = value;
}

double StatsFunctions::getHealthCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[0].mCurrent;
}

void StatsFunctions::setHealthCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[0].mCurrent = value;
}

double StatsFunctions::getMagickaBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[1].mBase;
}

void StatsFunctions::setMagickaBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[1].mBase = value;
}

double StatsFunctions::getMagickaCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[1].mCurrent;
}

void StatsFunctions::setMagickaCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[1].mCurrent = value;
}

double StatsFunctions::getFatigueBase(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[2].mBase;
}

void StatsFunctions::setFatigueBase(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[2].mBase = value;
}

double StatsFunctions::getFatigueCurrent(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->CreatureStats()->mDynamic[2].mCurrent;
}

void StatsFunctions::setFatigueCurrent(unsigned short pid, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[2].mCurrent = value;
}

int StatsFunctions::getAttributeCount() noexcept
{
    return Attribute::Length;
}

int StatsFunctions::getSkillCount() noexcept
{
    return Skill::Length;
}

int StatsFunctions::getAttributeId(const char *name) noexcept
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

int StatsFunctions::getSkillId(const char *name) noexcept
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

const char *StatsFunctions::getAttributeName(unsigned short attribute) noexcept
{
    if (attribute >= Attribute::Length)
        return "invalid";

    return Attribute::sAttributeNames[attribute].c_str();
}

const char *StatsFunctions::getSkillName(unsigned short skill) noexcept
{
    if (skill >= Skill::Length)
        return "invalid";

    return Skill::sSkillNames[skill].c_str();
}

int StatsFunctions::getAttributeBase(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute >= Attribute::Length)
        return 0;

    return player->CreatureStats()->mAttributes[attribute].mBase;
}

void StatsFunctions::setAttributeBase(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute >= Attribute::Length)
        return;

    player->CreatureStats()->mAttributes[attribute].mBase = value;
}

int StatsFunctions::getAttributeCurrent(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute >= Attribute::Length)
        return 0;

    return player->CreatureStats()->mAttributes[attribute].mCurrent;
}

void StatsFunctions::setAttributeCurrent(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute >= Attribute::Length)
        return;

    player->CreatureStats()->mAttributes[attribute].mCurrent = value;
}

int StatsFunctions::getSkillBase(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->NpcStats()->mSkills[skill].mBase;
}

void StatsFunctions::setSkillBase(unsigned short pid, unsigned short skill, int value) noexcept  //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill >= Skill::Length)
        return;

    player->NpcStats()->mSkills[skill].mBase = value;
}

int StatsFunctions::getSkillCurrent(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill >= Skill::Length)
        return 0;

    return player->NpcStats()->mSkills[skill].mCurrent;
}

void StatsFunctions::setSkillCurrent(unsigned short pid, unsigned short skill, int value) noexcept //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill >= Skill::Length)
        return;

    player->NpcStats()->mSkills[skill].mCurrent = value;
}

double StatsFunctions::getSkillProgress(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    if (skill >= Skill::Length)
        return 0;

    return player->NpcStats()->mSkills[skill].mProgress;
}

void StatsFunctions::setSkillProgress(unsigned short pid, unsigned short skill, double value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    if (skill >= Skill::Length)
        return;

    player->NpcStats()->mSkills[skill].mProgress = value;
}

int StatsFunctions::getSkillIncrease(unsigned short pid, unsigned int attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute > Attribute::Length)
        return 0;

    return player->NpcStats()->mSkillIncrease[attribute];
}

void StatsFunctions::setSkillIncrease(unsigned short pid, unsigned int attribute, int value) noexcept // TODO: need packet for transmit it
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute > Attribute::Length)
        return;

    player->NpcStats()->mSkillIncrease[attribute] = value;
}

void StatsFunctions::setCharGenStage(unsigned short pid, int start, int end) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CharGenStage()->current = start;
    player->CharGenStage()->end = end;

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_CHARGEN)->Send(player, false);
}

void StatsFunctions::resurrect(unsigned short pid)
{
    Player *player;
    GET_PLAYER(pid, player,);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_RESURRECT)->RequestData(player->guid);
}

void StatsFunctions::sendBaseInfo(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_BASE_INFO)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_BASE_INFO)->Send(player, true);
}

void StatsFunctions::sendDynamicStats(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_DYNAMICSTATS)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_DYNAMICSTATS)->Send(player, true);
}

void StatsFunctions::sendAttributes(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_ATTRIBUTE)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_ATTRIBUTE)->Send(player, true);
}

void StatsFunctions::sendSkills(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_SKILL)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_SKILL)->Send(player, true);
}

void StatsFunctions::sendLevel(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_LEVEL)->Send(player, false);
    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_LEVEL)->Send(player, true);
}
