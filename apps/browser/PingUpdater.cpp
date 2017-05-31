//
// Created by koncord on 02.05.17.
//

#include "PingUpdater.hpp"
#include "netutils/Utils.hpp"
#include <QDebug>
#include <QModelIndex>
#include <QThread>

void PingUpdater::stop()
{
    servers.clear();
    run = false;
}

void PingUpdater::addServer(int row, AddrPair addr)
{
    servers.push_back({row, addr});
    run = true;
    emit start();
}

void PingUpdater::process()
{
    while (run)
    {
        if (servers.count() == 0)
        {
            QThread::msleep(1000);
            if (servers.count() == 0)
            {
                qDebug() << "PingUpdater stopped due to inactivity";
                run = false;
                continue;
            }
        }

        ServerRow server = servers.back();
        servers.pop_back();

        unsigned ping = PingRakNetServer(server.second.first.toLatin1(), server.second.second);

        qDebug() << "Pong from" << server.second.first + "|" + QString::number(server.second.second)
                 << ":" << ping << "ms";

        emit updateModel(server.first, ping);
    }
    emit finished();
}
