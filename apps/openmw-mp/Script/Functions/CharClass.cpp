//
// Created by koncord on 29.08.16.
//

#include "CharClass.hpp"
#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace std;
using namespace ESM;

void CharClassFunctions::sendClass(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_CHARCLASS)->Send(player, false);
}

void CharClassFunctions::setDefaultClass(unsigned short pid, const char *id) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mId = id;
}
void CharClassFunctions::setClassName(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mName = name;
    player->charClass.mId = "";
}
void CharClassFunctions::setClassDesc(unsigned short pid, const char *desc) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mDescription = desc;
}
void CharClassFunctions::setClassMajorAttribute(unsigned short pid, unsigned char slot, int attrId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    if (slot > 1)
        throw invalid_argument("Incorrect attribute slot id");

    player->charClass.mData.mAttribute[slot] = attrId;

}
void CharClassFunctions::setClassSpecialization(unsigned short pid, int spec) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    player->charClass.mData.mSpecialization = spec;
}
void CharClassFunctions::setClassMajorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    player->charClass.mData.mSkills[slot][1] = skillId;
}
void CharClassFunctions::setClassMinorSkill(unsigned short pid, unsigned char slot, int skillId) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    player->charClass.mData.mSkills[slot][0] = skillId;
}

int CharClassFunctions::isClassDefault(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return !player->charClass.mId.empty(); // true if default
}

const char *CharClassFunctions::getDefaultClass(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,"");
    return player->charClass.mId.c_str();
}

const char *CharClassFunctions::getClassName(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,"");
    return player->charClass.mName.c_str();
}

const char *CharClassFunctions::getClassDesc(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,"");
    return player->charClass.mDescription.c_str();
}

int CharClassFunctions::getClassMajorAttribute(unsigned short pid, unsigned char slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    if (slot > 1)
        throw invalid_argument("Incorrect attribute slot id");
    return player->charClass.mData.mAttribute[slot];
}

int CharClassFunctions::getClassSpecialization(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->charClass.mData.mSpecialization;
}

int CharClassFunctions::getClassMajorSkill(unsigned short pid, unsigned char slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    return player->charClass.mData.mSkills[slot][1];
}

int CharClassFunctions::getClassMinorSkill(unsigned short pid, unsigned char slot) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    if (slot > 4)
        throw invalid_argument("Incorrect skill slot id");
    return player->charClass.mData.mSkills[slot][0];
}
