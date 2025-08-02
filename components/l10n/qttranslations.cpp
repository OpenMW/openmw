#include "qttranslations.hpp"

#include <QLibraryInfo>
#include <QLocale>

namespace L10n
{
    QTranslator AppTranslator{};
    QTranslator ComponentsTranslator{};
    QTranslator QtBaseAppTranslator{};

    void installQtTranslations(QApplication& app, const QString& appName, const QString& resourcesPath)
    {
        // Try to load OpenMW translations from resources folder first.
        // If we loaded them, try to load Qt translations from both
        // resources folder and default translations folder as well.
        auto qtPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        auto localPath = resourcesPath + "/translations";

        if (AppTranslator.load(QLocale::system(), appName, "_", localPath)
            && ComponentsTranslator.load(QLocale::system(), "components", "_", localPath))
        {
            app.installTranslator(&AppTranslator);
            app.installTranslator(&ComponentsTranslator);

            if (QtBaseAppTranslator.load(QLocale::system(), "qtbase", "_", localPath)
                || QtBaseAppTranslator.load(QLocale::system(), "qt", "_", localPath)
                || QtBaseAppTranslator.load(QLocale::system(), "qtbase", "_", qtPath)
                || QtBaseAppTranslator.load(QLocale::system(), "qt", "_", qtPath))
                app.installTranslator(&QtBaseAppTranslator);
        }
    }
}
