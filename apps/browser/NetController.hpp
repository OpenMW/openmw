//
// Created by koncord on 07.01.17.
//

#ifndef NEWLAUNCHER_NETCONTROLLER_HPP
#define NEWLAUNCHER_NETCONTROLLER_HPP


#include "ServerModel.hpp"
#include "netutils/HTTPNetwork.hpp"
#include "netutils/Utils.hpp"

struct ServerModel;

class NetController
{
public:
    static NetController *get();
    static void Create(std::string addr, unsigned short port);
    static void Destroy();
    bool updateInfo(QAbstractItemModel *pModel, QModelIndex index= QModelIndex());
    void updateInfo();
    QStringList players();
    QStringList plugins();
    void selectServer(ServerData *pServerData);
    ServerData *selectedServer();
protected:
    NetController(std::string addr, unsigned short port);
    ~NetController();
private:
    NetController(const NetController &controller);
    bool downloadInfo(QAbstractItemModel *pModel, QModelIndex index);
    void setData(QString addr, QJsonObject server, ServerModel *model);

    static NetController *mThis;
    ServerData *sd;
    HTTPNetwork httpNetwork;
    ServerExtendedData sed;
};


#endif //NEWLAUNCHER_NETCONTROLLER_HPP
