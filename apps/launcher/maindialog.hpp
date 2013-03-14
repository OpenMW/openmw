#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>

#include <components/files/configurationmanager.hpp>

#include "settings/gamesettings.hpp"
#include "settings/graphicssettings.hpp"
#include "settings/launchersettings.hpp"

#include "ui_mainwindow.h"

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QStringList;
class QStringListModel;
class QString;

class PlayPage;
class GraphicsPage;
class DataFilesPage;

class MainDialog : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainDialog();
    bool setup();
    bool showFirstRunDialog();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void play();

private:
    void createIcons();
    void createPages();

    bool setupLauncherSettings();
    bool setupGameSettings();
    bool setupGraphicsSettings();

    void loadSettings();
    void saveSettings();
    bool writeSettings();

    inline bool startProgram(const QString &name, bool detached = false) { return startProgram(name, QStringList(), detached); }
    bool startProgram(const QString &name, const QStringList &arguments, bool detached = false);

    void closeEvent(QCloseEvent *event);

    PlayPage *mPlayPage;
    GraphicsPage *mGraphicsPage;
    DataFilesPage *mDataFilesPage;

    Files::ConfigurationManager mCfgMgr;

    GameSettings mGameSettings;
    GraphicsSettings mGraphicsSettings;
    LauncherSettings mLauncherSettings;

};

#endif
