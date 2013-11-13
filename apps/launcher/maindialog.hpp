#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>
#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif
#include "settings/gamesettings.hpp"
#include "settings/graphicssettings.hpp"
#include "settings/launchersettings.hpp"

#include "ui_mainwindow.h"

class QListWidgetItem;

namespace Launcher
{
    class PlayPage;
    class GraphicsPage;
    class DataFilesPage;
    class UnshieldThread;

#ifndef WIN32
    bool expansions(Launcher::UnshieldThread& cd);
#endif

    class MainDialog : public QMainWindow, private Ui::MainWindow
    {
        Q_OBJECT

    public:
        explicit MainDialog(QWidget *parent = 0);
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
}
#endif
