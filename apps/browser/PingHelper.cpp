//
// Created by koncord on 03.05.17.
//

#include "PingHelper.hpp"
#include "ServerModel.hpp"
#include <QDebug>
#include "PingUpdater.hpp"

void PingHelper::Add(int row, AddrPair addrPair)
{
    pingUpdater->addServer(row, addrPair);
    if (!pingThread->isRunning())
        pingThread->start();
}

void PingHelper::Stop()
{
    emit pingUpdater->stop();
}

void PingHelper::SetModel(QAbstractTableModel *model)
{
    this->model = model;
}

void PingHelper::update(int row, unsigned ping)
{
    model->setData(model->index(row, ServerData::PING), ping);
}

PingHelper &PingHelper::Get()
{
    static PingHelper helper;
    return helper;
}

PingHelper::PingHelper()
{
    QObject();
    pingThread = new QThread;
    pingUpdater = new PingUpdater;
    pingUpdater->moveToThread(pingThread);

    connect(pingThread, SIGNAL(started()), pingUpdater, SLOT(process()));
    connect(pingUpdater, SIGNAL(start()), pingThread, SLOT(start()));
    connect(pingUpdater, SIGNAL(finished()), pingThread, SLOT(quit()));
    connect(this, SIGNAL(stop()), pingUpdater, SLOT(stop()));
    //connect(pingUpdater, SIGNAL(finished()), pingUpdater, SLOT(deleteLater()));
    connect(pingUpdater, SIGNAL(updateModel(int, unsigned)), this, SLOT(update(int, unsigned)));


}

PingHelper::~PingHelper()
{

}
