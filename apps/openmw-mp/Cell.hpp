//
// Created by koncord on 18.02.17.
//

#ifndef OPENMW_CELL_HPP
#define OPENMW_CELL_HPP

#include <deque>
#include <string>
#include <components/esm/records.hpp>
#include <components/openmw-mp/Base/BaseEvent.hpp>
#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

class Player;
class Cell;


class CellController
{
private:
    CellController();
    ~CellController();

    CellController(CellController&); // not used
public:
    static void create();
    static void destroy();
    static CellController *get();
public:
    typedef std::deque<Cell*> TContainer;
    typedef TContainer::iterator TIter;

    Cell * addCell(ESM::Cell cell);
    void removeCell(Cell *);

    void removePlayer(Cell *cell, Player *player);
    void deletePlayer(Player *player);

    Cell *getCell(ESM::Cell *esmCell);
    Cell *getCellByXY(int x, int y);
    Cell *getCellByName(std::string cellName);

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
    typedef TPlayers::const_iterator Iterator;

    Iterator begin() const;
    Iterator end() const;

    void addPlayer(Player *player);
    void removePlayer(Player *player);

    TPlayers getPlayers() const;
    void sendToLoaded(mwmp::WorldPacket *worldPacket, mwmp::BaseEvent *baseEvent) const;

    std::string getDescription() const;


private:
    TPlayers players;
    ESM::Cell cell;
};


#endif //OPENMW_CELL_HPP
