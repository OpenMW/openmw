#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>

#include <components/files/configurationmanager.hpp>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QStringList;
class QStringListModel;
class QString;

class PlayPage;
class GraphicsPage;
class DataFilesPage;

class GameSettings;
class GraphicsSettings;
class LauncherSettings;

class MainDialog : public QMainWindow
{
    Q_OBJECT

public:
    MainDialog(GameSettings &gameSettings,
               GraphicsSettings &GraphicsSettings,
               LauncherSettings &launcherSettings);

    bool setup();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void play();

private:
    void createIcons();
    void createPages();

    void loadSettings();
    void saveSettings();
    void writeSettings();

    void closeEvent(QCloseEvent *event);

    QListWidget *mIconWidget;
    QStackedWidget *mPagesWidget;

    PlayPage *mPlayPage;
    GraphicsPage *mGraphicsPage;
    DataFilesPage *mDataFilesPage;

    Files::ConfigurationManager mCfgMgr;

    GameSettings &mGameSettings;
    GraphicsSettings &mGraphicsSettings;
    LauncherSettings &mLauncherSettings;

};

#endif
