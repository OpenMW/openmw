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
#include <QtWidgets/QMessageBox>

using namespace std;

NetController *NetController::mThis = nullptr;

NetController *NetController::get()
{
    assert(mThis);
    return mThis;
}

void NetController::Create(std::string addr, unsigned short port)
{
    assert(!mThis);
    mThis = new NetController(addr, port);
}

void NetController::Destroy()
{
    assert(mThis);
    delete mThis;
    mThis = nullptr;
}

NetController::NetController(std::string addr, unsigned short port) : httpNetwork(addr, port)
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

bool NetController::downloadInfo(QAbstractItemModel *pModel, QModelIndex index)
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

    if(data == "UNKNOWN_ADDRESS")
    {
        QMessageBox::critical(0, "Error", "Cannot connect to the master server!");
        return false;
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

    return true;
}

bool NetController::updateInfo(QAbstractItemModel *pModel, QModelIndex index)
{
    qDebug() << "TODO: \"NetController::updateInfo(QAbstractItemModel *, QModelIndex)\" is not completed";
    ServerModel *model = ((ServerModel*)pModel);

    bool result;
    if (index.isValid() && index.row() >= 0)
    {
        //ServerData &sd = model->myData[index.row()];
        //qDebug() << sd.addr;
        result = downloadInfo(pModel, index);
    }
    else
    {
        for (QVector<ServerData>::Iterator iter = model->myData.begin(); iter != model->myData.end(); iter++)
        {
            qDebug() << iter->addr;
        }
        model->removeRows(0, model->rowCount(index));
        result = downloadInfo(pModel, index);
    }
    return result;
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

    if(data == "UNKNOWN_ADDRESS")
    {
        QMessageBox::critical(0, "Error", "Cannot connect to the master server!");
        return;
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

