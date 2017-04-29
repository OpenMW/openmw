//
// Created by koncord on 18.02.17.
//

#include "Cell.hpp"

#include <iostream>
#include "Player.hpp"

using namespace std;

Cell::Cell(ESM::Cell cell) : cell(cell)
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

void Cell::addPlayer(Player *player)
{
    auto it = find(player->cells.begin(), player->cells.end(), this);
    if (it == player->cells.end())
    {
        LOG_APPEND(Log::LOG_INFO, "- Adding %s to Player %s", getDescription().c_str(), player->npc.mName.c_str());

        player->cells.push_back(this);
    }

    LOG_APPEND(Log::LOG_INFO, "- Adding %s to Cell %s", player->npc.mName.c_str(), getDescription().c_str());

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
                LOG_APPEND(Log::LOG_INFO, "- Removing %s from Player %s", getDescription().c_str(), player->npc.mName.c_str());

                player->cells.erase(it2);
            }

            LOG_APPEND(Log::LOG_INFO, "- Removing %s from Cell %s", player->npc.mName.c_str(), getDescription().c_str());

            players.erase(it);
            return;
        }
    }
}

Cell::TPlayers Cell::getPlayers() const
{
    return players;
}

void Cell::sendToLoaded(mwmp::ActorPacket *actorPacket, mwmp::BaseActorList *baseActorList) const
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
        if (pl->guid == baseActorList->guid) continue;

        actorPacket->setActorList(baseActorList);

        // Send the packet to this eligible guid
        actorPacket->Send(pl->guid);
    }
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

        worldPacket->setEvent(baseEvent);

        // Send the packet to this eligible guid
        worldPacket->Send(pl->guid);
    }
}

std::string Cell::getDescription() const
{
    return cell.getDescription();
}
