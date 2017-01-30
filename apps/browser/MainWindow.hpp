//
// Created by koncord on 06.01.17.
//

#ifndef NEWLAUNCHER_MAIN_HPP
#define NEWLAUNCHER_MAIN_HPP


#include "ui_Main.h"
#include "ServerModel.hpp"
#include "MySortFilterProxyModel.hpp"
#include <components/process/processinvoker.hpp>

class MainWindow : public QMainWindow,  private Ui::MainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();
protected:
    void closeEvent(QCloseEvent * event) Q_DECL_OVERRIDE;
    void addServerAndUpdate(QString addr);
public slots:
    bool refresh();
protected slots:
    void tabSwitched(int index);
    void addServer();
    void addServerByIP();
    void deleteServer();
    void play();
    void serverSelected();
    void notFullSwitch(bool state);
    void havePlayersSwitch(bool state);
    void maxLatencyChanged(int index);
    void gamemodeChanged(const QString &text);
private:
    Process::ProcessInvoker *mGameInvoker;
    ServerModel *browser, *favorites;
    MySortFilterProxyModel *proxyModel;
    void loadFavorites();
};


#endif //NEWLAUNCHER_MAIN_HPP
