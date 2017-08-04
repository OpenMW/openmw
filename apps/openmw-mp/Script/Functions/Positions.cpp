#include "Positions.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>
using namespace std;

void PositionFunctions::GetPos(unsigned short pid, float *x, float *y, float *z) noexcept
{
    *x = 0.00;
    *y = 0.00;
    *z = 0.00;

    Player *player;
    GET_PLAYER(pid, player,);

    *x = player->position.pos[0];
    *y = player->position.pos[1];
    *z = player->position.pos[2];
}

double PositionFunctions::GetPosX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->position.pos[0];
}

double PositionFunctions::GetPosY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->position.pos[1];
}

double PositionFunctions::GetPosZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->position.pos[2];
}

double PositionFunctions::GetPreviousCellPosX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->previousCellPosition.pos[0];
}

double PositionFunctions::GetPreviousCellPosY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->previousCellPosition.pos[1];
}

double PositionFunctions::GetPreviousCellPosZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->previousCellPosition.pos[2];
}

void PositionFunctions::GetRot(unsigned short pid, float *x, float *y, float *z) noexcept
{
    *x = 0.00;
    *y = 0.00;
    *z = 0.00;

    Player *player;
    GET_PLAYER(pid, player, );

    *x = player->position.rot[0];
    *y = player->position.rot[1];
    *z = player->position.rot[2];
}

double PositionFunctions::GetRotX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->position.rot[0];
}

double PositionFunctions::GetRotZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->position.rot[2];
}

void PositionFunctions::SetPos(unsigned short pid, double x, double y, double z) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->position.pos[0] = x;
    player->position.pos[1] = y;
    player->position.pos[2] = z;
}

void PositionFunctions::SetRot(unsigned short pid, double x, double z) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->position.rot[0] = x;
    player->position.rot[2] = z;
}

void PositionFunctions::SendPos(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_POSITION)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_POSITION)->Send(false);
}
