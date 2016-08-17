//
// Created by koncord on 29.02.16.
//

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>
using namespace std;

void ScriptFunctions::GetPos(unsigned short pid, float *x, float *y, float *z) noexcept
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

double ScriptFunctions::GetPosX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->pos[0];
}

double ScriptFunctions::GetPosY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->pos[1];
}

double ScriptFunctions::GetPosZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->pos[2];
}

void ScriptFunctions::SetPos(unsigned short pid, double x, double y, double z) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->Position()->pos[0] = x;
    player->Position()->pos[1] = y;
    player->Position()->pos[2] = z;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_POS)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_POS)->Send(player, true);
}

void ScriptFunctions::SetCell(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    /*if (player->GetCell()->mName == name)
        return;*/

    cout << "attempt to move player (pid: " << pid << " name: " << player->Npc()->mName << ") from ";
    if (!player->GetCell()->isExterior())
        cout << "\"" << player->GetCell()->mName << "\"";
    else
        cout << "exterior";

    player->GetCell()->mName = name;

    cout << " in to cell \"" << player->GetCell()->mName << "\"" << endl;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_CELL)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_CELL)->Send(player, true);
}

const char* ScriptFunctions::GetCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);


    return player->GetCell()->mName.c_str();
}

void ScriptFunctions::SetExterior(unsigned short pid, int x, int y) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    cout << "attempt to move player (pid: " << pid << " name: " << player->Npc()->mName << ") from ";
    if (!player->GetCell()->isExterior())
        cout << "\"" << player->GetCell()->mName << "\"";
    else
        cout << "exterior: " << player->GetCell()->mCellId.mIndex.mX << ", " << player->GetCell()->mCellId.mIndex.mY;
    cout << " in to exterior cell \"" << x << ", " << y << "\"" << endl;

    player->GetCell()->mName = "";
    player->GetCell()->mCellId.mIndex.mX = x;
    player->GetCell()->mCellId.mIndex.mY = y;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_CELL)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_CELL)->Send(player, true);
}

int ScriptFunctions::GetExteriorX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->GetCell()->mCellId.mIndex.mX;
}

int ScriptFunctions::GetExteriorY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->GetCell()->mCellId.mIndex.mY;
}

bool ScriptFunctions::IsInInterior(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return !player->GetCell()->isExterior();
}

void ScriptFunctions::GetAngle(unsigned short pid, float *x, float *y, float *z) noexcept
{
    *x = 0.00;
    *y = 0.00;
    *z = 0.00;

    Player *player;
    GET_PLAYER(pid, player,);

    *x = player->Position()->rot[0];
    *y = player->Position()->rot[1];
    *z = player->Position()->rot[2];
}

double ScriptFunctions::GetAngleX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->rot[0];
}

double ScriptFunctions::GetAngleY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->rot[1];
}

double ScriptFunctions::GetAngleZ(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0.0f);

    return player->Position()->rot[2];
}

void ScriptFunctions::SetAngle(unsigned short pid, double x, double y, double z) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    player->Position()->rot[0] = x;
    player->Position()->rot[1] = y;
    player->Position()->rot[2] = z;

    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_POS)->Send(player, false);
    mwmp::Networking::Get().GetController()->GetPacket(ID_GAME_UPDATE_POS)->Send(player, true);
}
