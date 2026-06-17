#ifndef COMPONENTS_L10N_QTTRANSLATIONS_H
#define COMPONENTS_L10N_QTTRANSLATIONS_H

#include <QApplication>
#include <QTranslator>

namespace L10n
{
    extern QTranslator AppTranslator;
    extern QTranslator ComponentsTranslator;
    extern QTranslator QtBaseAppTranslator;

    void installQtTranslations(QApplication& app, const QString& appName, const QString& resourcesPath);
}

#endif // COMPONENTS_L10N_QTTRANSLATIONS_H
