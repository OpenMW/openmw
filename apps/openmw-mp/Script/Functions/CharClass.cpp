//
// Created by koncord on 29.08.16.
//

#include "CharClass.hpp"
#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace std;
using namespace ESM;

void CharClassFunctions::SendClass(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_PLAYER_CHARCLASS)->Send(player, false);
}

void CharClassFunctions::SetDefaultClass(unsigned short pid, const char *id) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mId = id;
}
void CharClassFunctions::SetClassName(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mName = name;
    player->charClass.mId = "";
}
void CharClassFunctions::SetClassDesc(unsigned short pid, const char *desc) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mDescription = desc;
}
void CharClassFunctions::SetClassMajorAttribute(unsigned short pid, unsigned char slot, int attrId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (slot > 1)
        throw invalid_argument("Incorrect attribute slot id");

    player->charClass.mData.mAttribute[slot] = attrId;

}
void CharClassFunctions::SetClassSpecialization(unsigned short pid, int spec) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mData.mSpecialization = spec;
}
void CharClassFunctions::SetClassMajorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    player->charClass.mData.mSkills[slot][1] = skillId;
}
void CharClassFunctions::SetClassMinorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    player->charClass.mData.mSkills[slot][0] = skillId;
}

int CharClassFunctions::IsClassDefault(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return !player->charClass.mId.empty(); // true if default
}

const char *CharClassFunctions::GetDefaultClass(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,"");
    return player->charClass.mId.c_str();
}

const char *CharClassFunctions::GetClassName(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,"");
    return player->charClass.mName.c_str();
}

const char *CharClassFunctions::GetClassDesc(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,"");
    return player->charClass.mDescription.c_str();
}

int CharClassFunctions::GetClassMajorAttribute(unsigned short pid, unsigned char slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    if (slot > 1)
        throw invalid_argument("Incorrect attribute slot id");
    return player->charClass.mData.mAttribute[slot];
}

int CharClassFunctions::GetClassSpecialization(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->charClass.mData.mSpecialization;
}

int CharClassFunctions::GetClassMajorSkill(unsigned short pid, unsigned char slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    return player->charClass.mData.mSkills[slot][1];
}

int CharClassFunctions::GetClassMinorSkill(unsigned short pid, unsigned char slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    return player->charClass.mData.mSkills[slot][0];
}
