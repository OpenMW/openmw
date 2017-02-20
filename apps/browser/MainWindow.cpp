//
// Created by koncord on 06.01.17.
//

#include "MainWindow.hpp"
#include "NetController.hpp"
#include "ServerInfoDialog.hpp"
#include "components/files/configurationmanager.hpp"
#include <qdebug.h>
#include <QInputDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>

using namespace Process;

MainWindow::MainWindow(QWidget *parent)
{
    setupUi(this);

    mGameInvoker = new ProcessInvoker();

    browser = new ServerModel;
    favorites = new ServerModel;
    proxyModel = new MySortFilterProxyModel(this);
    proxyModel->setSourceModel(browser);
    tblServerBrowser->setModel(proxyModel);
    tblFavorites->setModel(proxyModel);

    tblServerBrowser->hideColumn(ServerData::ADDR);
    tblFavorites->hideColumn(ServerData::ADDR);

    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabSwitched(int)));
    connect(actionAdd, SIGNAL(triggered(bool)), this, SLOT(addServer()));
    connect(actionAdd_by_IP, SIGNAL(triggered(bool)), this, SLOT(addServerByIP()));
    connect(actionDelete, SIGNAL(triggered(bool)), this, SLOT(deleteServer()));
    connect(actionRefresh, SIGNAL(triggered(bool)), this, SLOT(refresh()));
    connect(actionPlay, SIGNAL(triggered(bool)), this, SLOT(play()));
    connect(tblServerBrowser, SIGNAL(clicked(QModelIndex)), this, SLOT(serverSelected()));
    connect(tblFavorites, SIGNAL(clicked(QModelIndex)), this, SLOT(serverSelected()));
    connect(tblFavorites, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(play()));
    connect(tblServerBrowser, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(play()));
    connect(cBoxNotFull, SIGNAL(toggled(bool)), this, SLOT(notFullSwitch(bool)));
    connect(cBoxWithPlayers, SIGNAL(toggled(bool)), this, SLOT(havePlayersSwitch(bool)));
    connect(comboLatency, SIGNAL(currentIndexChanged(int)), this, SLOT(maxLatencyChanged(int)));
    connect(leGamemode, SIGNAL(textChanged(const QString &)), this, SLOT(gamemodeChanged(const QString &)));
    loadFavorites();
}

MainWindow::~MainWindow()
{
    delete mGameInvoker;
}

void MainWindow::addServerAndUpdate(QString addr)
{
    favorites->insertRow(0);
    QModelIndex mi = favorites->index(0, ServerData::ADDR);
    favorites->setData(mi, addr, Qt::EditRole);
    NetController::get()->updateInfo(favorites, mi);
}

void MainWindow::addServer()
{
    int id = tblServerBrowser->selectionModel()->currentIndex().row();

    if(id >= 0)
    {
        int sourceId = proxyModel->mapToSource(proxyModel->index(id, ServerData::ADDR)).row();
        favorites->myData.push_back(browser->myData[sourceId]);
    }
}

void MainWindow::addServerByIP()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add Server by address"), tr("Address:"), QLineEdit::Normal, "", &ok);
    if(ok && !text.isEmpty())
        addServerAndUpdate(text);
}

void MainWindow::deleteServer()
{
    if(tabWidget->currentIndex() != 1)
        return;
    int id = tblFavorites->selectionModel()->currentIndex().row();
    if(id >= 0)
    {
        int sourceId = proxyModel->mapToSource(proxyModel->index(id, ServerData::ADDR)).row();
        favorites->removeRow(sourceId);
        if(favorites->myData.isEmpty())
        {
            actionPlay->setEnabled(false);
            actionDelete->setEnabled(false);
        }
    }
}

bool MainWindow::refresh()
{
    return NetController::get()->updateInfo(proxyModel->sourceModel());
    /*tblServerBrowser->resizeColumnToContents(ServerData::HOSTNAME);
    tblServerBrowser->resizeColumnToContents(ServerData::MODNAME);
    tblFavorites->resizeColumnToContents(ServerData::HOSTNAME);
    tblFavorites->resizeColumnToContents(ServerData::MODNAME);*/
}

void MainWindow::play()
{
    QTableView *curTable = tabWidget->currentIndex() ? tblFavorites : tblServerBrowser;
    int id = curTable->selectionModel()->currentIndex().row();
    if(id < 0)
        return;

    ServerInfoDialog infoDialog(this);
    ServerModel *sm = ((ServerModel*)proxyModel->sourceModel());

    int sourceId = proxyModel->mapToSource(proxyModel->index(id, ServerData::ADDR)).row();
    NetController::get()->selectServer(&sm->myData[sourceId]);
    infoDialog.refresh();
    if(!infoDialog.exec())
        return;

    QStringList arguments;
    arguments.append(QLatin1String("--connect=") + sm->myData[sourceId].addr.toLatin1());

    if(sm->myData[sourceId].needPassw)
    {
        bool ok;
        QString passw = QInputDialog::getText(this, "Connecting to: " + sm->myData[sourceId].addr, "Password: ", QLineEdit::Password, "", &ok);
        if(!ok)
            return;
        arguments.append(QLatin1String("--password=") + passw.toLatin1());
    }

    if (mGameInvoker->startProcess(QLatin1String("tes3mp"), arguments, true))
        return qApp->quit();
}

void MainWindow::tabSwitched(int index)
{
    if(index == 0)
    {
        proxyModel->setSourceModel(browser);
        actionDelete->setEnabled(false);
    }
    else
    {
        proxyModel->setSourceModel(favorites);
    }
    actionPlay->setEnabled(false);
    actionAdd->setEnabled(false);
}

void MainWindow::serverSelected()
{
    actionPlay->setEnabled(true);
    if(tabWidget->currentIndex() == 0)
        actionAdd->setEnabled(true);
    if(tabWidget->currentIndex() == 1)
        actionDelete->setEnabled(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Files::ConfigurationManager cfgMgr;
    QString cfgPath = QString::fromStdString((cfgMgr.getUserConfigPath() / "favorites.dat").string());

    QJsonArray saveData;
    for(auto server : favorites->myData)
        saveData.push_back(server.addr);

    QFile file(cfgPath);

    if(!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Cannot save " << cfgPath;
        return;
    }

    file.write(QJsonDocument(saveData).toJson());
    file.close();
}


void MainWindow::loadFavorites()
{
    Files::ConfigurationManager cfgMgr;
    QString cfgPath = QString::fromStdString((cfgMgr.getUserConfigPath() / "favorites.dat").string());

    QFile file(cfgPath);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Cannot open " << cfgPath;
        return;
    }

    QJsonDocument jsonDoc(QJsonDocument::fromJson(file.readAll()));

    for(auto server : jsonDoc.array())
        addServerAndUpdate(server.toString());

    file.close();
}

void MainWindow::notFullSwitch(bool state)
{
    proxyModel->filterFullServer(state);
}

void MainWindow::havePlayersSwitch(bool state)
{
    proxyModel->filterEmptyServers(state);
}

void MainWindow::maxLatencyChanged(int index)
{
    int maxLatency = index * 50;
    proxyModel->pingLessThan(maxLatency);

}

void MainWindow::gamemodeChanged(const QString &text)
{
    proxyModel->setFilterFixedString(text);
    proxyModel->setFilterKeyColumn(ServerData::MODNAME);
}
