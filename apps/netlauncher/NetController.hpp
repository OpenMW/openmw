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
    static void Create();
    static void Destroy();
    void updateInfo(QAbstractItemModel *pModel, QModelIndex index= QModelIndex());
    void updateInfo();
    QStringList players();
    QStringList plugins();
    void selectServer(ServerData *pServerData);
    ServerData *selectedServer();
protected:
    NetController();
    ~NetController();
private:
    NetController(const NetController &controller);
    void downloadInfo(QAbstractItemModel *pModel, QModelIndex index);

    static NetController *mThis;
    ServerData *sd;
    HTTPNetwork httpNetwork;
    ServerExtendedData sed;
};


#endif //NEWLAUNCHER_NETCONTROLLER_HPP
