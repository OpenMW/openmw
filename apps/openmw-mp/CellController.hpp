#ifndef OPENMW_SERVERCELLCONTROLLER_HPP
#define OPENMW_SERVERCELLCONTROLLER_HPP

#include <deque>
#include <string>
#include <components/esm/records.hpp>
#include <components/openmw-mp/Base/BaseEvent.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
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

#endif //OPENMW_SERVERCELLCONTROLLER_HPP
