#include <iostream>

#include <QDir>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <components/debug/debugging.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/qtconversion.hpp>
#include <components/l10n/qttranslations.hpp>
#include <components/platform/application.hpp>
#include <components/platform/platform.hpp>

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

#include "maindialog.hpp"

int runLauncher(int argc, char* argv[])
{
    Platform::init();

    boost::program_options::variables_map variables;
    boost::program_options::options_description description;
    Files::ConfigurationManager configurationManager;
    configurationManager.addCommonOptions(description);
    configurationManager.readConfiguration(variables, description, true);

    Debug::setupLogging(configurationManager.getLogPath(), "Launcher");

    try
    {
        Platform::Application app(argc, argv);

        QString resourcesPath(".");
        if (!variables["resources"].empty())
        {
            resourcesPath = Files::pathToQString(variables["resources"].as<Files::MaybeQuotedPath>().u8string());
        }

        l10n::installQtTranslations(app, "launcher", resourcesPath);

        Launcher::MainDialog mainWin(configurationManager);

        Launcher::FirstRunDialogResult result = mainWin.showFirstRunDialog();
        if (result == Launcher::FirstRunDialogResultFailure)
            return 0;

        if (result == Launcher::FirstRunDialogResultContinue)
            mainWin.show();

        int exitCode = app.exec();

        return exitCode;
    }
    catch (const std::exception& e)
    {
        Log(Debug::Error) << "Unexpected exception: " << e.what();
        return 0;
    }
}

int main(int argc, char* argv[])
{
    return Debug::wrapApplication(runLauncher, argc, argv, "Launcher");
}
