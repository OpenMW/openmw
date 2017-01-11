//
// Created by koncord on 07.01.17.
//

#include <cassert>
#include <QtCore/QTime>
#include "NetController.hpp"
#include "qdebug.h"

#include <RakPeer.h>
#include <RakSleep.h>

#include <sstream>

#include <QJsonDocument>
#include <QJsonArray>
#include <memory>

using namespace std;

NetController *NetController::mThis = nullptr;

NetController *NetController::get()
{
    assert(mThis);
    return mThis;
}

void NetController::Create()
{
    assert(!mThis);
    mThis = new NetController;
}

void NetController::Destroy()
{
    assert(mThis);
    delete mThis;
    mThis = nullptr;
}

NetController::NetController() : httpNetwork("127.0.0.1", 8080)
{

}

NetController::~NetController()
{

}

struct pattern
{
    pattern(QString value): value(value) {}
    bool operator()(const ServerData &data)
    {
        return value == data.addr;
    }
    QString value;
};

void NetController::downloadInfo(QAbstractItemModel *pModel, QModelIndex index)
{
    ServerModel *model = ((ServerModel *) pModel);

    /*
     * download stuff
     */

    QString data;
    while (true)
    {
        data = QString::fromStdString(httpNetwork.getData("/api/servers"));
        if (!data.isEmpty() && data != "NO_CONTENT" && data != "LOST_CONNECTION")
            break;
        RakSleep(30);
    }

    qDebug() << "Content: " << data;

    QJsonParseError err;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data.toLatin1(), &err);

    QMap<QString, QVariant> map = jsonDocument.toVariant().toMap()["list servers"].toMap();

    for(QMap<QString, QVariant>::Iterator iter = map.begin(); iter != map.end(); iter++)
    {
        qDebug() << iter.key();
        qDebug() << iter.value().toMap()["hostname"].toString();
        qDebug() << iter.value().toMap()["modname"].toString();
        qDebug() << iter.value().toMap()["players"].toInt();
        qDebug() << iter.value().toMap()["max_players"].toInt();

        QVector<ServerData>::Iterator value = std::find_if(model->myData.begin(), model->myData.end(), pattern(iter.key()));
        if(value == model->myData.end())
            model->insertRow(0);

        QModelIndex mi = model->index(0, ServerData::ADDR);
        model->setData(mi, iter.key());

        mi = model->index(0, ServerData::PLAYERS);
        model->setData(mi, iter.value().toMap()["players"].toInt());

        mi = model->index(0, ServerData::MAX_PLAYERS);
        model->setData(mi, iter.value().toMap()["max_players"].toInt());

        mi = model->index(0, ServerData::HOSTNAME);
        model->setData(mi, iter.value().toMap()["hostname"].toString());

        mi = model->index(0, ServerData::MODNAME);
        model->setData(mi, iter.value().toMap()["modname"].toString());

        mi = model->index(0, ServerData::PING);

        QStringList addr = iter.key().split(":");
        model->setData(mi, PingRakNetServer(addr[0].toLatin1().data(), addr[1].toUShort()));
    }

    //qDebug() << data;

    if (model->rowCount() != 0)
        return;

    /*model->insertRows(0, 6);
    model->myData[0] = {"127.0.0.1:25565", 0, 20, 1, "Super Server"};
    model->myData[1] = {"127.0.0.1:25565", 5, 20, 2, "Koncord's server"};
    model->myData[2] = {"server.local:8888", 15, 15, 15, "tes3mp server", "custom mode"};
    model->myData[3] = {"127.0.0.1:25562", 1, 2, 5, "Server"};
    model->myData[4] = {"tes3mp.com:22222", 8, 9, 1000, "Antoher Server", "super duper mod"};
    model->myData[5] = {"localhost:24", 1, 5, 5, "Test Server", "Another mod 0.1"};*/
}

void NetController::updateInfo(QAbstractItemModel *pModel, QModelIndex index)
{
    qDebug() << "TODO: \"NetController::updateInfo(QAbstractItemModel *, QModelIndex)\" is not completed";
    ServerModel *model = ((ServerModel*)pModel);

    if (index.isValid() && index.row() >= 0)
    {
        //ServerData &sd = model->myData[index.row()];
        //qDebug() << sd.addr;
        downloadInfo(pModel, index);
    }
    else
    {
        for (QVector<ServerData>::Iterator iter = model->myData.begin(); iter != model->myData.end(); iter++)
        {
            qDebug() << iter->addr;
        }
        model->removeRows(0, model->rowCount(index));
        downloadInfo(pModel, index);
    }
}

void NetController::updateInfo()
{
    QString data;
    QString uri = "/api/servers/" + sd->addr;
    while (true)
    {
        data = QString::fromStdString(httpNetwork.getData(uri.toLatin1()));
        if (!data.isEmpty() && data != "NO_CONTENT" && data != "LOST_CONNECTION")
            break;
        RakSleep(30);
    }

    qDebug() << "Content: " << data;

    QJsonParseError err;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data.toLatin1(), &err);

    QMap<QString, QVariant> map = jsonDocument.toVariant().toMap()["server"].toMap();

    qDebug() << sd->addr;
    qDebug() << map["hostname"].toString();
    qDebug() << map["modname"].toString();
    qDebug() << map["players"].toInt();
    qDebug() << map["max_players"].toInt();

    sd->hostName = map["hostname"].toString();
    sd->modName = map["modname"].toString();
    sd->players = map["players"].toInt();
    sd->maxPlayers = map["max_players"].toInt();

    QStringList addr = sd->addr.split(":");
    sd->ping = PingRakNetServer(addr[0].toLatin1(), addr[1].toUShort());
    sed = getExtendedData(addr[0].toLatin1(), addr[1].toUShort());
}

QStringList NetController::players()
{
    QStringList listPlayers;
    for(vector<string>::iterator player = sed.players.begin(); player != sed.players.end(); player++)
        listPlayers.push_back(player->c_str());
    return listPlayers;
}

QStringList NetController::plugins()
{
    QStringList listPlugins;
    for(vector<string>::iterator plugin = sed.plugins.begin(); plugin != sed.plugins.end(); plugin++)
        listPlugins.push_back(plugin->c_str());
    return listPlugins;
}

void NetController::selectServer(ServerData *pServerData)
{
    sd = pServerData;
}

ServerData *NetController::selectedServer()
{
    return sd;
}

