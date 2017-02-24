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
#include <QJsonObject>
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

void NetController::setData(QString address, QJsonObject server, ServerModel *model)
{
    QModelIndex mi = model->index(0, ServerData::ADDR);
    model->setData(mi, address);

    mi = model->index(0, ServerData::PLAYERS);
    model->setData(mi, server["players"].toInt());

    mi = model->index(0, ServerData::MAX_PLAYERS);
    model->setData(mi, server["max_players"].toInt());

    mi = model->index(0, ServerData::HOSTNAME);
    model->setData(mi, server["hostname"].toString());

    mi = model->index(0, ServerData::MODNAME);
    model->setData(mi, server["modname"].toString());

    mi = model->index(0, ServerData::VERSION);
    model->setData(mi, server["version"].toString());

    mi = model->index(0, ServerData::PASSW);
    model->setData(mi, server["passw"].toBool());

    mi = model->index(0, ServerData::PING);
    
    // This *should* fix a crash when a port isn't returned by data.
    if(!address.contains(":"))
        address.append(":25565");
    QStringList addr = address.split(":");
    model->setData(mi, PingRakNetServer(addr[0].toLatin1().data(), addr[1].toUShort())); 
}

bool NetController::downloadInfo(QAbstractItemModel *pModel, QModelIndex index)
{
    ServerModel *model = ((ServerModel *) pModel);

    /*
     * download stuff
     */

    QString data;
    QJsonParseError err;

    if(index.isValid() && index.row() >= 0)
    {
        const ServerData &sd = model->myData[index.row()];
        while(true)
        {
            data = QString::fromStdString(httpNetwork.getData((QString("/api/servers/") + sd.addr).toLatin1()));
            if (!data.isEmpty() && data != "NO_CONTENT" && data != "LOST_CONNECTION")
                break;
            RakSleep(30);
        }
        qDebug() << "Content for \"" <<  sd.addr << "\": " << data;

        if(data == "bad request" || data == "not found") // TODO: if server is not registered we should download info directly from the server
        {
            qDebug() << "Server is not registered";
            return false;
        }

        QJsonDocument jsonDocument = QJsonDocument::fromJson(data.toLatin1(), &err);
        QJsonObject server = jsonDocument.object()["server"].toObject();

        setData(sd.addr, server, model);

        return true;
    }

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

    QJsonDocument jsonDocument = QJsonDocument::fromJson(data.toLatin1(), &err);

    QJsonObject listServers = jsonDocument.object()["list servers"].toObject();

    for(auto iter = listServers.begin(); iter != listServers.end(); iter++)
    {
        QJsonObject server = iter->toObject();
        qDebug() << iter.key();
        qDebug() << server["hostname"].toString();
        qDebug() << server["modname"].toString();
        qDebug() << server["players"].toInt();
        qDebug() << server["max_players"].toInt();
        qDebug() << server["version"].toString();
        qDebug() << server["passw"].toBool();

        QVector<ServerData>::Iterator value = std::find_if(model->myData.begin(), model->myData.end(), pattern(iter.key()));
        if(value == model->myData.end())
            model->insertRow(0);

        setData(iter.key(), server, model);
    }

    return true;
}

bool NetController::updateInfo(QAbstractItemModel *pModel, QModelIndex index)
{
    ServerModel *model = ((ServerModel*)pModel);

    bool result;
    if (index.isValid() && index.row() >= 0)
        result = downloadInfo(pModel, index);
    else
    {
        for (auto iter = model->myData.begin(); iter != model->myData.end(); iter++)
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
    qDebug() << map["version"].toString();
    qDebug() << map["passw"].toBool();

    sd->hostName = map["hostname"].toString();
    sd->modName = map["modname"].toString();
    sd->players = map["players"].toInt();
    sd->maxPlayers = map["max_players"].toInt();

    if(!sd->addr.contains(":")) 
        sd->addr.append(":25565");
    QStringList addr = sd->addr.split(":");
    sd->ping = PingRakNetServer(addr[0].toLatin1(), addr[1].toUShort());
    if(sd->ping != PING_UNREACHABLE)
        sed = getExtendedData(addr[0].toLatin1(), addr[1].toUShort());
    else
        qDebug() << "Server is unreachable";
}

QStringList NetController::players()
{
    QStringList listPlayers;
    for(auto player = sed.players.begin(); player != sed.players.end(); player++)
        listPlayers.push_back(player->c_str());
    return listPlayers;
}

QStringList NetController::plugins()
{
    QStringList listPlugins;
    for(auto plugin = sed.plugins.begin(); plugin != sed.plugins.end(); plugin++)
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

