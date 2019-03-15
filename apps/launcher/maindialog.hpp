#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QMainWindow>
#include <QProcess>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>


#include <components/process/processinvoker.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>

#include <components/settings/settings.hpp>
#endif
#include "ui_mainwindow.h"

class QListWidgetItem;
class QStackedWidget;
class QStringList;
class QStringListModel;
class QString;

namespace Launcher
{
    class PlayPage;
    class GraphicsPage;
    class DataFilesPage;
    class UnshieldThread;
    class SettingsPage;
    class AdvancedPage;

    enum FirstRunDialogResult
    {
        FirstRunDialogResultFailure,
        FirstRunDialogResultContinue,
        FirstRunDialogResultWizard
    };

#ifndef WIN32
    bool expansions(Launcher::UnshieldThread& cd);
#endif

    class MainDialog : public QMainWindow, private Ui::MainWindow
    {
        Q_OBJECT

    public:
        explicit MainDialog(QWidget *parent = 0);
        ~MainDialog();

        FirstRunDialogResult showFirstRunDialog();

        bool reloadSettings();
        bool writeSettings();

    public slots:
        void changePage(QListWidgetItem *current, QListWidgetItem *previous);
        void play();

    private slots:
        void wizardStarted();
        void wizardFinished(int exitCode, QProcess::ExitStatus exitStatus);

    private:
        bool setup();

        void createIcons();
        void createPages();

        bool setupLauncherSettings();
        bool setupGameSettings();
        bool setupGraphicsSettings();
        bool setupGameData();

        void setVersionLabel();

        void loadSettings();
        void saveSettings();

        inline bool startProgram(const QString &name, bool detached = false) { return startProgram(name, QStringList(), detached); }
        bool startProgram(const QString &name, const QStringList &arguments, bool detached = false);

        void closeEvent(QCloseEvent *event);

        PlayPage *mPlayPage;
        GraphicsPage *mGraphicsPage;
        DataFilesPage *mDataFilesPage;
        SettingsPage *mSettingsPage;
        AdvancedPage *mAdvancedPage;

        Process::ProcessInvoker *mGameInvoker;
        Process::ProcessInvoker *mWizardInvoker;

        Files::ConfigurationManager mCfgMgr;

        Config::GameSettings mGameSettings;
        Settings::Manager mEngineSettings;
        Config::LauncherSettings mLauncherSettings;

    };
}
#endif
