#include <iostream>

#include <QDir>
#include <QTranslator>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <components/debug/debugging.hpp>
#include <components/files/configurationmanager.hpp>
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

    setupLogging(configurationManager.getLogPath(), "Launcher");

    try
    {
        QApplication app(argc, argv);

        // Internationalization
        QString locale = QLocale::system().name().section('_', 0, 0);

        QTranslator appTranslator;
        appTranslator.load(":/translations/" + locale + ".qm");
        app.installTranslator(&appTranslator);

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
    return wrapApplication(runLauncher, argc, argv, "Launcher");
}
