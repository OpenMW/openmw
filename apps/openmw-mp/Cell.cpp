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
    {
        LOG_APPEND(Log::LOG_INFO, "- Adding %s to Player %s",
            getDescription().c_str(),
            player->npc.mName.c_str());

        player->cells.push_back(this);
    }

    LOG_APPEND(Log::LOG_INFO, "- Adding %s to Cell %s",
        player->npc.mName.c_str(),
        getDescription().c_str());

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
            {
                LOG_APPEND(Log::LOG_INFO, "- Removing %s from Player %s",
                    getDescription().c_str(),
                    player->npc.mName.c_str());

                player->cells.erase(it2);
            }

            LOG_APPEND(Log::LOG_INFO, "- Removing %s from Cell %s",
                player->npc.mName.c_str(),
                getDescription().c_str());

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
    for (auto cell : cells)
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
    auto it = find_if(cells.begin(), cells.end(), [x, y](const Cell *c)
    {
        return c->cell.mData.mX == x && c->cell.mData.mY == y;
    });

    if (it == cells.end())
    {
        LOG_APPEND(Log::LOG_INFO, "- Attempt to get Cell at %i, %i failed!", x, y);
        return nullptr;
    }

    return *it;
}

Cell *CellController::getCellByName(std::string cellName)
{
    auto it = find_if(cells.begin(), cells.end(), [cellName](const Cell *c)
    {
        return c->cell.mName == cellName;
    });

    if (it == cells.end())
    {
        LOG_APPEND(Log::LOG_INFO, "- Attempt to get Cell at %s failed!", cellName.c_str());
        return nullptr;
    }

    return *it;
}

Cell *CellController::addCell(ESM::Cell cellData)
{
    LOG_APPEND(Log::LOG_INFO, "- Loaded cells: %d",
        cells.size());
    auto it = find_if(cells.begin(), cells.end(), [cellData](const Cell *c) {
        //return c->cell.sRecordId == cellData.sRecordId; // Currently we cannot compare because plugin lists can be loaded in different order
        return c->cell.isExterior() ? (c->cell.mData.mX == cellData.mData.mX && c->cell.mData.mY == cellData.mData.mY) :
               (c->cell.mName == cellData.mName);
    });

    Cell *cell;
    if (it == cells.end())
    {
        LOG_APPEND(Log::LOG_INFO, "- Adding %s to CellController",
            cellData.getDescription().c_str());

        cell = new Cell(cellData);
        cells.push_back(cell);
    }
    else
    {
        LOG_APPEND(Log::LOG_INFO, "- Found %s in CellController",
            cellData.getDescription().c_str());
        cell = *it;
    }

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
            LOG_APPEND(Log::LOG_INFO, "- Removing %s from CellController",
                cell->getDescription().c_str());

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
        LOG_APPEND(Log::LOG_INFO, "- Deleting empty cell from memory: %s",
            cell->getDescription().c_str());
        auto it = find(cells.begin(), cells.end(), cell);
        delete *it;
        cells.erase(it);
    }
}

void CellController::deletePlayer(Player *player)
{
    LOG_APPEND(Log::LOG_INFO, "- Iterating through Cells from Player %s",
        player->npc.mName.c_str());

    for_each(player->getCells()->begin(), player->getCells()->end(), [&player](Cell *cell)
    {
        LOG_APPEND(Log::LOG_INFO, "-- Found Cell %s",
            cell->getDescription().c_str());

        for (auto it = cell->begin(); it != cell->end(); ++it)
        {
            if (*it == player)
            {
                LOG_APPEND(Log::LOG_INFO, "-- Deleting %s from Cell %s",
                    player->npc.mName.c_str(),
                    cell->getDescription().c_str());

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
