#include "Cells.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/Log.hpp>

#include <iostream>
using namespace std;

unsigned int CellFunctions::GetCellStateChangesSize(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->cellStateChanges.count;
}

unsigned int CellFunctions::GetCellStateType(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->cellStateChanges.cellStates.at(i).type;
}

const char *CellFunctions::GetCellStateDescription(unsigned short pid, unsigned int i) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");

    if (i >= player->cellStateChanges.count)
        return "invalid";

    string cellDescription = player->cellStateChanges.cellStates.at(i).cell.getDescription();

    static vector<char> cstrDescription;
    cstrDescription.reserve(cellDescription.size() + 1);
    strncpy(&cstrDescription[0], cellDescription.c_str(), cstrDescription.capacity());

    return &cstrDescription[0];
}

const char *CellFunctions::GetCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->cell.mName.c_str();
}

void CellFunctions::SetCell(unsigned short pid, const char *name) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Script is moving %s from %s to %s",
        player->npc.mName.c_str(),
        player->cell.getDescription().c_str(),
        name);

    // If the player is currently in an exterior, turn on the interior flag
    // from the  cell so the player doesn't get teleported to their exterior
    // grid position (which we haven't changed)
    if (player->cell.isExterior()) {
        player->cell.mData.mFlags |= ESM::Cell::Interior;
    }

    player->cell.mName = name;
}

void CellFunctions::SetExterior(unsigned short pid, int x, int y) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Script is moving %s from %s to %i,%i",
        player->npc.mName.c_str(),
        player->cell.getDescription().c_str(),
        x,
        y);

    // If the player is currently in an interior, turn off the interior flag
    // from the cell
    if (!player->cell.isExterior()) {
        player->cell.mData.mFlags &= ~ESM::Cell::Interior;
    }

    player->cell.mData.mX = x;
    player->cell.mData.mY = y;
}

int CellFunctions::GetExteriorX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->cell.mData.mX;
}

int CellFunctions::GetExteriorY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,0);
    return player->cell.mData.mY;
}

bool CellFunctions::IsInExterior(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return player->cell.isExterior();
}

void CellFunctions::SendCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerController()->GetPacket(ID_PLAYER_CELL_CHANGE)->Send(player, false);
}
