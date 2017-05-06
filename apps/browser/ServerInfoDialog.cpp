//
// Created by koncord on 07.01.17.
//

#include <apps/browser/netutils/QueryClient.hpp>
#include "qdebug.h"

#include "ServerInfoDialog.hpp"
#include <apps/browser/netutils/Utils.hpp>
#include <algorithm>

using namespace std;
using namespace RakNet;

ServerInfoDialog::ServerInfoDialog(QWidget *parent): QDialog(parent)
{
    setupUi(this);
    connect(btnRefresh, SIGNAL(clicked()), this, SLOT(refresh()));
}

ServerInfoDialog::~ServerInfoDialog()
{

}

void ServerInfoDialog::Server(QString addr)
{
    this->addr = addr;
}

void ServerInfoDialog::refresh()
{
    QStringList list = addr.split(':');
    auto sd = QueryClient::Get().Update(SystemAddress(list[0].toLatin1(), list[1].toUShort()));
    if (sd.first != UNASSIGNED_SYSTEM_ADDRESS)
    {
        leAddr->setText(sd.first.ToString(true, ':'));
        lblName->setText(sd.second.GetName());
        lblPing->setNum((int) PingRakNetServer(sd.first.ToString(false), sd.first.GetPort()));

        listPlayers->clear();

        for(auto player : sd.second.players)
        {
            listPlayers->addItem(QString::fromStdString(player));
        };

        listPlugins->clear();
        for(auto plugin : sd.second.plugins)
        {
            listPlugins->addItem(QString::fromStdString(plugin.name));
        }

        listRules->clear();
        const static vector<std::string> defaultRules {"gamemode", "maxPlayers", "name", "passw", "players", "version"};
        for (auto rule : sd.second.rules)
        {
            if(::find(defaultRules.begin(), defaultRules.end(), rule.first) != defaultRules.end())
                continue;
            QString ruleStr = QString::fromStdString(rule.first) + " : ";
            if(rule.second.type == 's')
                ruleStr += QString::fromStdString(rule.second.str);
            else
                ruleStr += QString::number(rule.second.val);
            listRules->addItem(ruleStr);
        }

        lblPlayers->setText(QString::number(sd.second.players.size()) + " / " + QString::number(sd.second.GetMaxPlayers()));
    }
}
