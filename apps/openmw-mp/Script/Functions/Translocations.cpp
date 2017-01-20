//
// Created by koncord on 29.02.16.
//
#include "Translocations.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/Log.hpp>

#include <iostream>
using namespace std;

void TranslocationFunctions::GetPos(unsigned short pid, float *x, float *y, float *z) noexcept
{
    *x = 0.00;
    *y = 0.00;
    *z = 0.00;

    Player *player;
    GET_PLAYER(pid, player,);

    *x = player->Position()->pos[0];
    *y = player->Position()->pos[1];
    *z = player->Position()->pos[2];
}

double TranslocationFunctions::GetPosX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->pos[0];
}

double TranslocationFunctions::GetPosY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->pos[1];
}

double TranslocationFunctions::GetPosZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->pos[2];
}

void TranslocationFunctions::GetAngle(unsigned short pid, float *x, float *y, float *z) noexcept
{
    *x = 0.00;
    *y = 0.00;
    *z = 0.00;

    Player *player;
    GET_PLAYER(pid, player, );

    *x = player->Position()->rot[0];
    *y = player->Position()->rot[1];
    *z = player->Position()->rot[2];
}

double TranslocationFunctions::GetAngleX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->rot[0];
}

double TranslocationFunctions::GetAngleY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->rot[1];
}

double TranslocationFunctions::GetAngleZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->rot[2];
}

void TranslocationFunctions::SetPos(unsigned short pid, double x, double y, double z) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->Position()->pos[0] = x;
    player->Position()->pos[1] = y;
    player->Position()->pos[2] = z;
}

void TranslocationFunctions::SetAngle(unsigned short pid, double x, double y, double z) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->Position()->rot[0] = x;
    player->Position()->rot[1] = y;
    player->Position()->rot[2] = z;
}

const char* TranslocationFunctions::GetCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->getCell()->mName.c_str();
}

void TranslocationFunctions::SetCell(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Script is moving %s from %s to %s",
        player->Npc()->mName.c_str(),
        player->getCell()->getDescription().c_str(),
        name);

    // If the player is currently in an exterior, turn on the interior flag
    // from the  cell so the player doesn't get teleported to their exterior
    // grid position (which we haven't changed)
    if (player->getCell()->isExterior()) {
        player->getCell()->mData.mFlags |= ESM::Cell::Interior;
    }

    player->getCell()->mName = name;
}

void TranslocationFunctions::SetExterior(unsigned short pid, int x, int y) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Script is moving %s from %s to %i,%i",
        player->Npc()->mName.c_str(),
        player->getCell()->getDescription().c_str(),
        x,
        y);

    // If the player is currently in an interior, turn off the interior flag
    // from the cell
    if (!player->getCell()->isExterior()) {
        player->getCell()->mData.mFlags &= ~ESM::Cell::Interior;
    }

    player->getCell()->mData.mX = x;
    player->getCell()->mData.mY = y;
}

int TranslocationFunctions::GetExteriorX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->getCell()->mData.mX;
}

int TranslocationFunctions::GetExteriorY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->getCell()->mData.mY;
}

bool TranslocationFunctions::IsInExterior(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return player->getCell()->isExterior();
}

void TranslocationFunctions::SendPos(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_GAME_POS)->Send(player, false);
}

void TranslocationFunctions::SendCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_PLAYER_CELL_CHANGE)->Send(player, false);
}

