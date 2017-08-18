#include "Cells.hpp"

#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>
using namespace std;

static std::string tempCellDescription;

void CellFunctions::InitializeMapChanges(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    player->mapChanges.cellsExplored.clear();
}

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

    tempCellDescription = player->cellStateChanges.cellStates.at(i).cell.getDescription();
    return tempCellDescription.c_str();
}

const char *CellFunctions::GetCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    tempCellDescription = player->cell.getDescription().c_str();
    return tempCellDescription.c_str();
}

int CellFunctions::GetExteriorX(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);
    return player->cell.mData.mX;
}

int CellFunctions::GetExteriorY(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);
    return player->cell.mData.mY;
}

bool CellFunctions::IsInExterior(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return player->cell.isExterior();
}

const char *CellFunctions::GetRegion(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, 0);

    return player->cell.mRegion.c_str();
}

bool CellFunctions::IsChangingRegion(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, false);

    return player->isChangingRegion;
}

void CellFunctions::SetCell(unsigned short pid, const char *cellDescription) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Script is moving %s from %s to %s", player->npc.mName.c_str(),
                       player->cell.getDescription().c_str(), cellDescription);

    player->cell = Utils::getCellFromDescription(cellDescription);
}

void CellFunctions::SetExteriorCell(unsigned short pid, int x, int y) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Script is moving %s from %s to %i,%i", player->npc.mName.c_str(),
                       player->cell.getDescription().c_str(), x, y);

    // If the player is currently in an interior, turn off the interior flag
    // from the cell
    if (!player->cell.isExterior())
        player->cell.mData.mFlags &= ~ESM::Cell::Interior;

    player->cell.mData.mX = x;
    player->cell.mData.mY = y;
}

void CellFunctions::AddCellExplored(unsigned short pid, const char* cellDescription) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    ESM::Cell cellExplored = Utils::getCellFromDescription(cellDescription);
    player->mapChanges.cellsExplored.push_back(cellExplored);
}

void CellFunctions::SendCell(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_CELL_CHANGE)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_CELL_CHANGE)->Send(false);
}

void CellFunctions::SendMapChanges(unsigned short pid, bool toOthers) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, );

    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_MAP)->setPlayer(player);
    mwmp::Networking::get().getPlayerPacketController()->GetPacket(ID_PLAYER_MAP)->Send(toOthers);
}
