//
// Created by koncord on 07.01.17.
//

#include "qdebug.h"
#include "NetController.hpp"

#include "ServerInfoDialog.hpp"

ServerInfoDialog::ServerInfoDialog(QWidget *parent): QDialog(parent)
{
    setupUi(this);
    connect(btnRefresh, SIGNAL(clicked()), this, SLOT(refresh()));
}

ServerInfoDialog::~ServerInfoDialog()
{

}

void ServerInfoDialog::refresh()
{
    NetController::get()->updateInfo();
    ServerData *sd = NetController::get()->selectedServer();
    if (sd)
    {
        leAddr->setText(sd->addr);
        lblName->setText(sd->hostName);
        lblPing->setNum(sd->ping);

        listPlayers->clear();
        QStringList players = NetController::get()->players();
        listPlayers->addItems(players);
        listPlugins->clear();
        listPlugins->addItems(NetController::get()->plugins());

        lblPlayers->setText(QString::number(players.size()) + " / " + QString::number(sd->maxPlayers));
    }
}
