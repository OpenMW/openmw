//
// Created by koncord on 18.02.17.
//

#include "Cell.hpp"

#include <iostream>
#include "Player.hpp"

using namespace std;

void Cell::AddPlayer(Player *player)
{
    auto it = find(player->cells.begin(), player->cells.end(), this);
    if(it == player->cells.end())
        player->cells.push_back(this);
    players.push_back(player);
}

void Cell::RemovePlayer(Player *player)
{
    for(Iterator it = players.begin(); it != players.end(); it++)
    {
        if(*it == player)
        {
            auto it2 = find(player->cells.begin(), player->cells.end(), this);
            if(it2 != player->cells.end())
                player->cells.erase(it2);
            players.erase(it);
            return;
        }

    }

}

Cell::TPlayers Cell::getPlayers()
{
    return players;
}

CellController::CellController()
{

}

CellController::~CellController()
{

}

CellController *CellController::sThis = nullptr;

void CellController::Create()
{
    sThis = new CellController;
}

void CellController::Destroy()
{
    assert(sThis);
    delete sThis;
    sThis = nullptr;
}

CellController *CellController::Get()
{
    return sThis;
}

Cell *CellController::GetCellByXY(int x, int y)
{
    auto it = find_if(cells.begin(), cells.end(), [x, y](const Cell *c) {
        return c->cell.mData.mX == x && c->cell.mData.mY == y;
    });
    if(it == cells.end())
        return nullptr;
    return *it;
}

Cell *CellController::GetCellByID(std::string cellid)
{
    auto it = find_if(cells.begin(), cells.end(), [cellid](const Cell *c) {
        return c->cell.mName == cellid;
    });
    if(it == cells.end())
        return nullptr;
    return *it;
}

Cell *CellController::AddCell(ESM::Cell cellData)
{
    
    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Loaded cells: %d", cells.size());
    auto it = find_if(cells.begin(), cells.end(), [cellData](const Cell *c) {
        //return c->cell.sRecordId == cellData.sRecordId; // Currently we cannot compare because plugin lists can be loaded in different order
        return c->cell.mData.mX == cellData.mData.mX && c->cell.mData.mY == cellData.mData.mY &&
                c->cell.mCellId.mWorldspace == cellData.mCellId.mWorldspace;
    });
    Cell *cell;
    if(it == cells.end())
    {
        cell = new Cell(cellData);
        cells.push_back(cell);
    }
    else
        cell = *it;
    return cell;


}

void CellController::RemoveCell(Cell *cell)
{
    if(cell == nullptr)
        return;
    for (auto it = cells.begin(); it != cells.end();)
    {
        if(*it != nullptr && *it == cell)
        {
            delete *it;
            it = cells.erase(it);
        }
        else
            ++it;
    }
}

void CellController::RemovePlayer(Cell *cell, Player *player)
{

    cell->RemovePlayer(player);

    if(cell->players.empty())
    {
        auto it = find(cells.begin(), cells.end(), cell);
        delete *it;
        cells.erase(it);
    }
}

void CellController::update(Player *player)
{
    for(auto cell : player->cellStateChanges.cellStates)
    {
        if(cell.type == mwmp::CellState::LOAD)
        {
            Cell *c = AddCell(cell.cell);
            c->AddPlayer(player);
        }
        else
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Unload cell: %d %d %s", cell.cell.mData.mX, cell.cell.mData.mY, cell.cell.mName.c_str());
            Cell *c;
            if(!cell.cell.isExterior())
                c = GetCellByID(cell.cell.mName);
            else
                c = GetCellByXY(cell.cell.getGridX(), cell.cell.getGridY());

            if(c != nullptr)
                RemovePlayer(c, player);
        }
    }
}

Cell::Cell(ESM::Cell cell): cell(cell)
{

}
