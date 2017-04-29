//
// Created by koncord on 18.02.17.
//

#ifndef OPENMW_SERVERCELL_HPP
#define OPENMW_SERVERCELL_HPP

#include <deque>
#include <string>
#include <components/esm/records.hpp>
#include <components/openmw-mp/Base/BaseEvent.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
#include <components/openmw-mp/Packets/World/WorldPacket.hpp>

class Player;
class Cell;

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
    void sendToLoaded(mwmp::ActorPacket *actorPacket, mwmp::BaseActorList *baseActorList) const;
    void sendToLoaded(mwmp::WorldPacket *worldPacket, mwmp::BaseEvent *baseEvent) const;

    std::string getDescription() const;


private:
    TPlayers players;
    ESM::Cell cell;
};


#endif //OPENMW_SERVERCELL_HPP
