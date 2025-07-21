#include <QDir>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <components/files/qtconversion.hpp>
#include <components/l10n/qttranslations.hpp>
#include <components/platform/application.hpp>

#include "mainwizard.hpp"

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#undef MAC_OS_X_VERSION_MIN_REQUIRED
// We need to do this because of Qt: https://bugreports.qt-project.org/browse/QTBUG-22154
#define MAC_OS_X_VERSION_MIN_REQUIRED __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#endif // MAC_OS_X_VERSION_MIN_REQUIRED

int main(int argc, char* argv[])
{
    boost::program_options::variables_map variables;
    boost::program_options::options_description description;
    Files::ConfigurationManager configurationManager;
    configurationManager.addCommonOptions(description);
    configurationManager.readConfiguration(variables, description, true);

    Platform::Application app(argc, argv);

    // Now we make sure the current dir is set to application path
    QDir dir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MAC
    // force Qt to load only LOCAL plugins, don't touch system Qt installation
    QDir pluginsPath(QCoreApplication::applicationDirPath());
    pluginsPath.cdUp();
    pluginsPath.cd("Plugins");

    QStringList libraryPaths;
    libraryPaths << pluginsPath.path() << QCoreApplication::applicationDirPath();
    app.setLibraryPaths(libraryPaths);
#endif

    QString resourcesPath(".");
    if (!variables["resources"].empty())
    {
        resourcesPath = Files::pathToQString(variables["resources"].as<Files::MaybeQuotedPath>().u8string());
    }

    L10n::installQtTranslations(app, "wizard", resourcesPath);

    Wizard::MainWizard wizard(std::move(configurationManager));

    wizard.show();
    return app.exec();
}
