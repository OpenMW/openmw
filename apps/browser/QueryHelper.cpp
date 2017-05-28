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
    qRegisterMetaType<QueryData>("QueryData");
    queryThread = new QThread;
    queryUpdate = new QueryUpdate;
    _model = model;
    connect(queryThread, SIGNAL(started()), queryUpdate, SLOT(process()));
    connect(queryUpdate, SIGNAL(finished()), queryThread, SLOT(quit()));
    connect(queryUpdate, &QueryUpdate::finished, [this](){emit finished();});
    connect(queryUpdate, SIGNAL(updateModel(QString, unsigned short, QueryData)),
            this, SLOT(update(QString, unsigned short, QueryData)));
    queryUpdate->moveToThread(queryThread);
}

void QueryHelper::refresh()
{
    if(!queryThread->isRunning())
        _model->removeRows(0, _model->rowCount());
    queryThread->start();
    emit started();
}

void QueryHelper::terminate()
{
    queryThread->terminate();
}

void QueryHelper::update(QString addr, unsigned short port, QueryData data)
{
    ServerModel *model = ((ServerModel*)_model);
    model->insertRow(model->rowCount());
    int row = model->rowCount() - 1;

    QModelIndex mi = model->index(row, ServerData::ADDR);
    model->setData(mi, addr + ":" + QString::number(port));

    mi = model->index(row, ServerData::PLAYERS);
    model->setData(mi, (int)data.players.size());

    mi = model->index(row, ServerData::MAX_PLAYERS);
    model->setData(mi, data.GetMaxPlayers());

    mi = model->index(row, ServerData::HOSTNAME);
    model->setData(mi, data.GetName());

    mi = model->index(row, ServerData::MODNAME);
    model->setData(mi, data.GetGameMode());

    mi = model->index(row, ServerData::VERSION);
    model->setData(mi, data.GetVersion());

    mi = model->index(row, ServerData::PASSW);
    model->setData(mi, data.GetPassword() == 1);

    mi = model->index(row, ServerData::PING);
    model->setData(mi, PING_UNREACHABLE);
    PingHelper::Get().Add(row, {addr, port});
}

void QueryUpdate::process()
{
    auto data = QueryClient::Get().Query();
    if(QueryClient::Get().Status() != ID_MASTER_QUERY)
    {
        emit finished();
        return;
    }

    for(auto server : data)
        emit updateModel(server.first.ToString(false), server.first.GetPort(), server.second);
    emit finished();
}
