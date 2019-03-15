#ifndef MAINWIZARD_HPP
#define MAINWIZARD_HPP

#include <QProcess>
#include <QWizard>
#include <QMap>

#include <components/process/processinvoker.hpp>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>

#include <components/config/gamesettings.hpp>
#include <components/config/launchersettings.hpp>
#endif

namespace Wizard
{
    class MainWizard : public QWizard
    {
        Q_OBJECT

    public:
        struct Installation {
            bool hasMorrowind;
            bool hasTribunal;
            bool hasBloodmoon;

            QString iniPath;
        };

        enum {
            Page_Intro,
            Page_MethodSelection,
            Page_LanguageSelection,
            Page_ExistingInstallation,
            Page_InstallationTarget,
            Page_ComponentSelection,
            Page_Installation,
            Page_Import,
            Page_Conclusion
        };

        MainWizard(QWidget *parent = 0);
        ~MainWizard();

        bool findFiles(const QString &name, const QString &path);
        void addInstallation(const QString &path);
        void runSettingsImporter();

        QMap<QString, Installation> mInstallations;

        Files::ConfigurationManager mCfgMgr;

        Process::ProcessInvoker *mImporterInvoker;

        bool mError;

    public slots:
        void addLogText(const QString &text);

    private:
        /// convert boost::filesystem::path to QString
        QString toQString(const boost::filesystem::path& path);

        void setupLog();
        void setupGameSettings();
        void setupLauncherSettings();
        void setupInstallations();
        void setupPages();

        void writeSettings();

        Config::GameSettings mGameSettings;
        Config::LauncherSettings mLauncherSettings;

        QString mLogError;

    private slots:

        void importerStarted();
        void importerFinished(int exitCode, QProcess::ExitStatus exitStatus);

        void accept();
        void reject();

    };

}

#endif // MAINWIZARD_HPP
