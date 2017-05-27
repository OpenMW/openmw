//
// Created by koncord on 27.05.17.
//

#include "netutils/QueryClient.hpp"
#include "netutils/Utils.hpp"
#include "QueryHelper.hpp"
#include "PingHelper.hpp"

QueryUpdate *queryUpdate;

QueryHelper::QueryHelper(QAbstractItemModel *model)
{
    queryThread = new QThread;
    queryUpdate = new QueryUpdate;
    queryUpdate->_model = model;
    connect(queryThread, SIGNAL(started()), queryUpdate, SLOT(process()));
    connect(queryUpdate, SIGNAL(finished()), queryThread, SLOT(quit()));
    connect(queryUpdate, &QueryUpdate::finished, [this](){emit finished();});
    queryUpdate->moveToThread(queryThread);
}

void QueryHelper::refresh()
{
    queryThread->start();
    emit started();
}

void QueryHelper::terminate()
{
    queryThread->terminate();
}

void QueryUpdate::process()
{
    auto data = QueryClient::Get().Query();
    if(QueryClient::Get().Status() != ID_MASTER_QUERY)
        return;

    ServerModel *model = ((ServerModel*)_model);
    model->removeRows(0, model->rowCount());
    for(auto server : data)
    {
        model->insertRow(model->rowCount());
        int row = model->rowCount() - 1;

        QModelIndex mi = model->index(row, ServerData::ADDR);
        model->setData(mi, server.first.ToString(true, ':'));

        mi = model->index(row, ServerData::PLAYERS);
        model->setData(mi, (int)server.second.players.size());

        mi = model->index(row, ServerData::MAX_PLAYERS);
        model->setData(mi, server.second.GetMaxPlayers());

        mi = model->index(row, ServerData::HOSTNAME);
        model->setData(mi, server.second.GetName());

        mi = model->index(row, ServerData::MODNAME);
        model->setData(mi, server.second.GetGameMode());

        mi = model->index(row, ServerData::VERSION);
        model->setData(mi, server.second.GetVersion());

        mi = model->index(row, ServerData::PASSW);
        model->setData(mi, server.second.GetPassword() == 1);

        mi = model->index(row, ServerData::PING);
        model->setData(mi, PING_UNREACHABLE);

        PingHelper::Get().Add(row, {server.first.ToString(false), server.first.GetPort()});
    }
    emit finished();
}
