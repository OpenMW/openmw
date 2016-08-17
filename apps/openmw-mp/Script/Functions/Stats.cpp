//
// Created by koncord on 29.02.16.
//

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/esm/attr.hpp>
#include <components/esm/loadskil.hpp>
#include <components/misc/stringops.hpp>
#include <iostream>

using namespace std;
using namespace ESM;

void ScriptFunctions::SetName(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->GetCell()->mName == name)
        return;

    player->GetCell()->mName = name;
}

const char *ScriptFunctions::GetName(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);

    cout << "pname: " << player->Npc()->mName.c_str() << endl;

    return player->Npc()->mName.c_str();
}

void ScriptFunctions::SetBirthsign(unsigned short pid, const char *sign) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->GetCell()->mName == sign)
        return;

    *player->BirthSign() = sign;
}

const char *ScriptFunctions::GetBirthsign(unsigned short pid) noexcept
{

    Player *player;
    GET_PLAYER(pid, player, 0);


    return player->BirthSign()->c_str();
}

void ScriptFunctions::SetRace(unsigned short pid, const char *race) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mRace == race)
        return;

    printf("Attempt to set race %s -> %s", player->Npc()->mRace.c_str(), race);

    player->Npc()->mRace = race;
}

const char *ScriptFunctions::GetRace(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mRace.c_str();
}

void ScriptFunctions::SetHead(unsigned short pid, const char *race) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mHead == race)
        return;

    player->Npc()->mHead = race;
}

const char *ScriptFunctions::GetHead(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);


    return player->Npc()->mHead.c_str();
}

void ScriptFunctions::SetHairstyle(unsigned short pid, const char *style) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (player->Npc()->mHair == style)
        return;

    player->Npc()->mHair = style;

}

const char *ScriptFunctions::GetHairstyle(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->Npc()->mHair.c_str();
}

int ScriptFunctions::GetIsMale(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,false);

    return player->Npc()->isMale();

}

void ScriptFunctions::SetIsMale(unsigned short pid, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->Npc()->setIsMale(value == true);
}


float ScriptFunctions::GetHealth(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0.f);

    return player->CreatureStats()->mDynamic[0].mBase;

}

void ScriptFunctions::SetHealth(unsigned short pid, float value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[0].mBase = value;
}

float ScriptFunctions::GetCurrentHealth(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.f);

    return player->CreatureStats()->mDynamic[0].mCurrent;
}

void ScriptFunctions::SetCurrentHealth(unsigned short pid, float value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[0].mCurrent = 0;
}

float ScriptFunctions::GetMagicka(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0.f);

    return player->CreatureStats()->mDynamic[1].mBase;
}

void ScriptFunctions::SetMagicka(unsigned short pid, float value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[1].mBase = value;
}

float ScriptFunctions::GetCurrentMagicka(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.f);

    return player->CreatureStats()->mDynamic[1].mCurrent;
}

void ScriptFunctions::SetCurrentMagicka(unsigned short pid, float value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[1].mCurrent = 0;
}

float ScriptFunctions::GetFatigue(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0.f);

    return player->CreatureStats()->mDynamic[2].mBase;

}

void ScriptFunctions::SetFatigue(unsigned short pid, float value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[2].mBase = value;
}

float ScriptFunctions::GetCurrentFatigue(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.f);

    return player->CreatureStats()->mDynamic[2].mCurrent;
}

void ScriptFunctions::SetCurrentFatigue(unsigned short pid, float value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CreatureStats()->mDynamic[2].mCurrent = 0;
}

int ScriptFunctions::GetAttributeId(const char *name) noexcept
{
    for (int x = 0; x < 8; x++)
    {
        if (Misc::StringUtils::ciEqual(name, Attribute::sAttributeNames[x]))
        {
            return x;
        }
    }

    return -1;
}

int ScriptFunctions::GetSkillId(const char *name) noexcept
{
    for (int x = 0; x < 27; x++)
    {
        if (Misc::StringUtils::ciEqual(name, Skill::sSkillNames[x]))
        {
            return x;
        }
    }

    return -1;
}

const char *ScriptFunctions::GetAttributeName(unsigned short attribute) noexcept
{
    return Attribute::sAttributeNames[attribute].c_str();
}

const char *ScriptFunctions::GetSkillName(unsigned short skill) noexcept
{
    return Skill::sSkillNames[skill].c_str();
}

int ScriptFunctions::GetAttribute(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute > 7)
        return 0;

    return player->CreatureStats()->mAttributes[attribute].mBase;
}

void ScriptFunctions::SetAttribute(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute > 7)
        return;

    DEBUG_PRINTF("SetAttribute(%d, %d, %d)\n", pid, attribute, value);

    player->CreatureStats()->mAttributes[attribute].mBase = value;
}

int ScriptFunctions::GetCurrentAttribute(unsigned short pid, unsigned short attribute) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (attribute > 7)
        return 0;

    return player->CreatureStats()->mAttributes[attribute].mCurrent;
}

void ScriptFunctions::SetCurrentAttribute(unsigned short pid, unsigned short attribute, int value) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (attribute > 7)
        return;

    player->CreatureStats()->mAttributes[attribute].mCurrent = value;
}

int ScriptFunctions::GetSkill(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill > 27)
        return 0;

    return player->NpcStats()->mSkills[skill].mBase;
}

void ScriptFunctions::SetSkill(unsigned short pid, unsigned short skill, int value) noexcept  //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill > 27)
        return;

    player->NpcStats()->mSkills[skill].mBase = value;

    //DEBUG_PRINTF("SetSkill(%d, %d, %d)\n", pid, skill, value);

}

int ScriptFunctions::GetCurrentSkill(unsigned short pid, unsigned short skill) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (skill > 27)
        return 0;

    return player->NpcStats()->mSkills[skill].mCurrent;
}

void ScriptFunctions::SetCurrentSkill(unsigned short pid, unsigned short skill, int value) noexcept //TODO: need packet for one value
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (skill > 27)
        return;

    player->NpcStats()->mSkills[skill].mCurrent = value;
}


int ScriptFunctions::GetIncreaseSkill(unsigned short pid, unsigned int pos) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    if (pos > 7)
        return 0;

    return player->NpcStats()->mSkillIncrease[pos];
}

void ScriptFunctions::SetIncreaseSkill(unsigned short pid, unsigned int pos, int value) noexcept // TODO: need packet for transmit it
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (pos > 7)
        return;

    player->NpcStats()->mSkillIncrease[pos] = value;
}

void ScriptFunctions::SetCharGenStage(unsigned short pid, int start, int end) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->CharGenStage()->current = start;
    player->CharGenStage()->end = end;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_CHARGEN)->Send(player, false);
}

void ScriptFunctions::Resurrect(unsigned short pid)
{
    Player *player;
    GET_PLAYER(pid, player,);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_RESURRECT)->RequestData(player->guid);
}

void ScriptFunctions::SendBaseInfo(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_BASE_INFO)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_BASE_INFO)->Send(player, true);
}

void ScriptFunctions::SendAttributes(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_ATTRIBUTE)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_ATTRIBUTE)->Send(player, true);
}

void ScriptFunctions::SendBaseStats(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_BASESTATS)->Send(player, true);
}

void ScriptFunctions::SendSkills(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_SKILL)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_SKILL)->Send(player, true);
}
