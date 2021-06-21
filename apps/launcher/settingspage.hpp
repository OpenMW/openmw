#ifndef SETTINGSPAGE_HPP
#define SETTINGSPAGE_HPP

#include <components/process/processinvoker.hpp>

#include "ui_settingspage.h"

#include "maindialog.hpp"

namespace Files { struct ConfigurationManager; }
namespace Config { class GameSettings;
                   class LauncherSettings; }

namespace Launcher
{
    class TextInputDialog;

    class SettingsPage : public QWidget, private Ui::SettingsPage
    {
        Q_OBJECT

    public:
        SettingsPage(Files::ConfigurationManager &cfg, Config::GameSettings &gameSettings,
                      Config::LauncherSettings &launcherSettings, MainDialog *parent = nullptr);
        ~SettingsPage() override;

        void saveSettings();
        bool loadSettings();
        
        /// set progress bar on page to 0%
        void resetProgressBar();

    private slots:

        void on_wizardButton_clicked();
        void on_importerButton_clicked();
        void on_browseButton_clicked();

        void wizardStarted();
        void wizardFinished(int exitCode, QProcess::ExitStatus exitStatus);

        void importerStarted();
        void importerFinished(int exitCode, QProcess::ExitStatus exitStatus);

        void updateOkButton(const QString &text);

    private:

        Process::ProcessInvoker *mWizardInvoker;
        Process::ProcessInvoker *mImporterInvoker;

        Files::ConfigurationManager &mCfgMgr;

        Config::GameSettings &mGameSettings;
        Config::LauncherSettings &mLauncherSettings;

        MainDialog *mMain;
        TextInputDialog *mProfileDialog;

    };
}

#endif // SETTINGSPAGE_HPP
