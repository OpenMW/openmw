#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <QMap>

#include "launchersettings.hpp"

LauncherSettings::LauncherSettings()
{
}

LauncherSettings::~LauncherSettings()
{
}


bool LauncherSettings::writeFile(QTextStream &stream)
{
    QString sectionPrefix;
    QRegExp sectionRe("([^/]+)/(.+)$");
    QMap<QString, QString> settings = SettingsBase::getSettings();

    QMapIterator<QString, QString> i(settings);
    while (i.hasNext()) {
        i.next();

        QString prefix;
        QString key;

        if (sectionRe.exactMatch(i.key())) {
             prefix = sectionRe.cap(1);
             key = sectionRe.cap(2);
        }

        if (sectionPrefix != prefix) {
            sectionPrefix = prefix;
            stream << "\n[" << prefix << "]\n";
        }

        stream << key << "=" << i.value() << "\n";
    }

    return true;

}
