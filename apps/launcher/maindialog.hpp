#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>

#include <components/files/configurationmanager.hpp>

#include "settings/gamesettings.hpp"
#include "settings/graphicssettings.hpp"
#include "settings/launchersettings.hpp"

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QStringList;
class QStringListModel;
class QString;

class PlayPage;
class GraphicsPage;
class DataFilesPage;

class MainDialog : public QMainWindow
{
    Q_OBJECT

public:
    MainDialog();


//    GameSettings &gameSettings,
//               GraphicsSettings &GraphicsSettings,
//               LauncherSettings &launcherSettings);

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
    void writeSettings();

    inline bool startProgram(const QString &name, bool detached = false) { return startProgram(name, QStringList(), detached); }
    bool startProgram(const QString &name, const QStringList &arguments, bool detached = false);

    void closeEvent(QCloseEvent *event);

    QListWidget *mIconWidget;
    QStackedWidget *mPagesWidget;

    PlayPage *mPlayPage;
    GraphicsPage *mGraphicsPage;
    DataFilesPage *mDataFilesPage;

    Files::ConfigurationManager mCfgMgr;

    GameSettings mGameSettings;
    GraphicsSettings mGraphicsSettings;
    LauncherSettings mLauncherSettings;

};

#endif
