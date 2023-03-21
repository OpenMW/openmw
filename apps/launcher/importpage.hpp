#ifndef IMPORTSPAGE_HPP
#define IMPORTSPAGE_HPP

#include <components/process/processinvoker.hpp>

#include "ui_importpage.h"

#include "maindialog.hpp"

namespace Files
{
    struct ConfigurationManager;
}
namespace Config
{
    class GameSettings;
    class LauncherSettings;
}

namespace Launcher
{
    class TextInputDialog;

    class ImportPage : public QWidget, private Ui::ImportPage
    {
        Q_OBJECT

    public:
        explicit ImportPage(const Files::ConfigurationManager& cfg, Config::GameSettings& gameSettings,
            Config::LauncherSettings& launcherSettings, MainDialog* parent = nullptr);
        ~ImportPage() override;

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

    private:
        Process::ProcessInvoker* mWizardInvoker;
        Process::ProcessInvoker* mImporterInvoker;

        const Files::ConfigurationManager& mCfgMgr;

        Config::GameSettings& mGameSettings;
        Config::LauncherSettings& mLauncherSettings;

        MainDialog* mMain;
    };
}

#endif // IMPORTSPAGE_HPP
