//
// Created by koncord on 30.01.17.
//

#include "MySortFilterProxyModel.hpp"
#include "ServerModel.hpp"

#include <qdebug.h>

bool MySortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

    QModelIndex pingIndex = sourceModel()->index(sourceRow, ServerData::PING, sourceParent);
    QModelIndex plIndex = sourceModel()->index(sourceRow, ServerData::PLAYERS, sourceParent);
    QModelIndex maxPlIndex = sourceModel()->index(sourceRow, ServerData::MAX_PLAYERS, sourceParent);

    int ping = sourceModel()->data(pingIndex).toInt();
    int players =  sourceModel()->data(plIndex).toInt();
    int maxPlayers =  sourceModel()->data(maxPlIndex).toInt();

    if(maxPing > 0 && (ping == -1 || ping > maxPing))
        return false;
    if(filterEmpty && players == 0)
        return false;
    if(filterFull && players >= maxPlayers)
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    filterEmpty = false;
    filterFull = false;
    maxPing = 0;
}

void MySortFilterProxyModel::filterEmptyServers(bool state)
{
    filterEmpty = state;
    invalidateFilter();
}

void MySortFilterProxyModel::filterFullServer(bool state)
{
    filterFull = state;
    invalidateFilter();
}

void MySortFilterProxyModel::pingLessThan(int maxPing)
{
    this->maxPing = maxPing;
    invalidateFilter();
}
