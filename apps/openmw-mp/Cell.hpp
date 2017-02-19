//
// Created by koncord on 18.02.17.
//

#ifndef OPENMW_CELL_HPP
#define OPENMW_CELL_HPP

#include <deque>
#include <string>
#include <components/esm/records.hpp>

class Player;
class Cell;


class CellController
{
private:
    CellController();
    ~CellController();

    CellController(CellController&); // not used
public:
    static void Create();
    static void Destroy();
    static CellController *Get();
public:
    typedef std::deque<Cell*> TContainer;
    typedef TContainer::iterator TIter;

    Cell * AddCell(ESM::Cell cell);
    void RemoveCell(Cell *);

    void RemovePlayer(Cell *cell, Player *player);

    Cell *GetCellByXY(int x, int y);
    Cell *GetCellByID(std::string cellid);

    void update(Player *player);

private:
    static CellController *sThis;
    TContainer cells;
};

class Cell
{
    friend class CellController;
public:
    Cell(ESM::Cell cell);
    typedef std::deque<Player*> TPlayers;
    typedef TPlayers::iterator Iterator;

    void AddPlayer(Player *player);
    void RemovePlayer(Player *player);

    TPlayers getPlayers();
private:
    TPlayers players;
    ESM::Cell cell;
};


#endif //OPENMW_CELL_HPP
