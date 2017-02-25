//
// Created by koncord on 18.02.17.
//

#include "Cell.hpp"

#include <iostream>
#include "Player.hpp"

using namespace std;

void Cell::addPlayer(Player *player)
{
    auto it = find(player->cells.begin(), player->cells.end(), this);
    if (it == player->cells.end())
        player->cells.push_back(this);
    players.push_back(player);
}

void Cell::removePlayer(Player *player)
{
    for (Iterator it = begin(); it != end(); it++)
    {
        if (*it == player)
        {
            auto it2 = find(player->cells.begin(), player->cells.end(), this);
            if (it2 != player->cells.end())
                player->cells.erase(it2);
            players.erase(it);
            return;
        }
    }
}

Cell::TPlayers Cell::getPlayers() const
{
    return players;
}

void Cell::sendToLoaded(mwmp::WorldPacket *worldPacket, mwmp::BaseEvent *baseEvent) const
{
    if (players.empty())
        return;

    std::list <Player*> plList;

    for (auto pl : players)
        plList.push_back(pl);

    plList.sort();
    plList.unique();

    for (auto pl : plList)
    {
        if (pl->guid == baseEvent->guid) continue;
        worldPacket->Send(baseEvent, pl->guid);
    }
}

std::string Cell::getDescription() const
{
    return cell.getDescription();
}

CellController::CellController()
{

}

CellController::~CellController()
{
    for(auto cell : cells)
    {
        delete cell;
    }
}

CellController *CellController::sThis = nullptr;

void CellController::create()
{
    assert(!sThis);
    sThis = new CellController;
}

void CellController::destroy()
{
    assert(sThis);
    delete sThis;
    sThis = nullptr;
}

CellController *CellController::get()
{
    assert(sThis);
    return sThis;
}

Cell *CellController::getCell(ESM::Cell *esmCell)
{
    if (esmCell->isExterior())
        return getCellByXY(esmCell->mData.mX, esmCell->mData.mY);
    else
        return getCellByName(esmCell->mName);
}


Cell *CellController::getCellByXY(int x, int y)
{
    auto it = find_if(cells.begin(), cells.end(), [x, y](const Cell *c) {
        return c->cell.mData.mX == x && c->cell.mData.mY == y;
    });
    if (it == cells.end())
        return nullptr;
    return *it;
}

Cell *CellController::getCellByName(std::string cellName)
{
    auto it = find_if(cells.begin(), cells.end(), [cellName](const Cell *c) {
        return c->cell.mName == cellName;
    });
    if (it == cells.end())
        return nullptr;
    return *it;
}

Cell *CellController::addCell(ESM::Cell cellData)
{
    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Loaded cells: %d", cells.size());
    auto it = find_if(cells.begin(), cells.end(), [cellData](const Cell *c) {
        //return c->cell.sRecordId == cellData.sRecordId; // Currently we cannot compare because plugin lists can be loaded in different order
        return c->cell.isExterior() ? (c->cell.mData.mX == cellData.mData.mX && c->cell.mData.mY == cellData.mData.mY) :
               (c->cell.mName == cellData.mName);
    });
    Cell *cell;
    if (it == cells.end())
    {
        cell = new Cell(cellData);
        cells.push_back(cell);
    }
    else
        cell = *it;
    return cell;


}

void CellController::removeCell(Cell *cell)
{
    if (cell == nullptr)
        return;
    for (auto it = cells.begin(); it != cells.end();)
    {
        if (*it != nullptr && *it == cell)
        {
            delete *it;
            it = cells.erase(it);
        }
        else
            ++it;
    }
}

void CellController::removePlayer(Cell *cell, Player *player)
{
    cell->removePlayer(player);

    if (cell->players.empty())
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Deleting empty cell from memory: %s", player->npc.mName.c_str(),
                           player->getId(), cell->cell.getDescription().c_str());
        auto it = find(cells.begin(), cells.end(), cell);
        delete *it;
        cells.erase(it);
    }
}

void CellController::deletePlayer(Player *player)
{

    for_each(player->getCells()->begin(), player->getCells()->end(), [&player](Cell *cell) {
        for (auto it = cell->begin(); it != cell->end(); ++it)
        {
            if (*it == player)
            {
                cell->players.erase(it);
                break;
            }
        }
    });
}

void CellController::update(Player *player)
{
    for (auto cell : player->cellStateChanges.cellStates)
    {
        if (cell.type == mwmp::CellState::LOAD)
        {
            Cell *c = addCell(cell.cell);
            c->addPlayer(player);
        }
        else
        {
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Player %s (%d) unloaded cell: %s", player->npc.mName.c_str(), player->getId(), cell.cell.getDescription().c_str());
            Cell *c;
            if (!cell.cell.isExterior())
                c = getCellByName(cell.cell.mName);
            else
                c = getCellByXY(cell.cell.getGridX(), cell.cell.getGridY());

            if (c != nullptr)
                removePlayer(c, player);
        }
    }
}

Cell::Cell(ESM::Cell cell): cell(cell)
{

}

Cell::Iterator Cell::begin() const
{
    return players.begin();
}

Cell::Iterator Cell::end() const
{
    return players.end();
}
