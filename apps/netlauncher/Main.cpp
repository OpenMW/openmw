//
// Created by koncord on 06.01.17.
//

#include "Main.hpp"
#include "NetController.hpp"
#include "ServerInfoDialog.hpp"
#include <qdebug.h>
#include <QInputDialog>

using namespace Process;

Main::Main(QWidget *parent)
{
    setupUi(this);

    mGameInvoker = new ProcessInvoker();

    browser = new ServerModel;
    favorites = new ServerModel;
    proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(browser);
    tblServerBrowser->setModel(proxyModel);
    tblFavorites->setModel(proxyModel);

    tblServerBrowser->hideColumn(ServerData::ADDR);
    tblFavorites->hideColumn(ServerData::ADDR);

    refresh();

    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabSwitched(int)));
    connect(actionAdd, SIGNAL(triggered(bool)), this, SLOT(addServer()));
    connect(actionAdd_by_IP, SIGNAL(triggered(bool)), this, SLOT(addServerByIP()));
    connect(actionDelete, SIGNAL(triggered(bool)), this, SLOT(deleteServer()));
    connect(actionRefresh, SIGNAL(triggered(bool)), this, SLOT(refresh()));
    connect(actionPlay, SIGNAL(triggered(bool)), this, SLOT(play()));
    connect(tblServerBrowser, SIGNAL(clicked(QModelIndex)), this, SLOT(serverSelected()));
    connect(tblFavorites, SIGNAL(clicked(QModelIndex)), this, SLOT(serverSelected()));
}

Main::~Main()
{
    delete mGameInvoker;
}

void Main::addServer()
{
    int id = tblServerBrowser->selectionModel()->currentIndex().row();

    if(id >= 0)
    {
        int sourceId = proxyModel->mapToSource(proxyModel->index(id, ServerData::ADDR)).row();
        favorites->myData.push_back(browser->myData[sourceId]);
    }
}

void Main::addServerByIP()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add Server by address"), tr("Address:"), QLineEdit::Normal, "", &ok);
    if(ok && !text.isEmpty())
    {
        favorites->insertRows(0, 1);
        QModelIndex mi = favorites->index(0, ServerData::ADDR);
        favorites->setData(mi, text, Qt::EditRole);
        NetController::get()->updateInfo(favorites, mi);
    }
}

void Main::deleteServer()
{
    if(tabWidget->currentIndex() != 1)
        return;
    int id = tblFavorites->selectionModel()->currentIndex().row();
    if(id >= 0)
    {
        int sourceId = proxyModel->mapToSource(proxyModel->index(id, ServerData::ADDR)).row();
        favorites->removeRow(sourceId);
        if(favorites->myData.isEmpty())
            actionPlay->setEnabled(false);
    }
}

void Main::refresh()
{
    NetController::get()->updateInfo(proxyModel->sourceModel());
    /*tblServerBrowser->resizeColumnToContents(ServerData::HOSTNAME);
    tblServerBrowser->resizeColumnToContents(ServerData::MODNAME);
    tblFavorites->resizeColumnToContents(ServerData::HOSTNAME);
    tblFavorites->resizeColumnToContents(ServerData::MODNAME);*/
}

void Main::play()
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

    if (mGameInvoker->startProcess(QLatin1String("tes3mp"), arguments, true))
        return qApp->quit();
}

void Main::tabSwitched(int index)
{
    if(index == 0)
    {
        proxyModel->setSourceModel(browser);
        actionDelete->setEnabled(false);
    }
    else
    {
        proxyModel->setSourceModel(favorites);
        actionDelete->setEnabled(true);
    }
    actionPlay->setEnabled(false);
    actionAdd->setEnabled(false);
}

void Main::serverSelected()
{
    actionPlay->setEnabled(true);
    if(tabWidget->currentIndex() == 0)
        actionAdd->setEnabled(true);
}
