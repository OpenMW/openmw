#include "launchersettings.hpp"

#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <QMap>

LauncherSettings::LauncherSettings()
{
}

LauncherSettings::~LauncherSettings()
{
}

QStringList LauncherSettings::values(const QString &key, Qt::MatchFlags flags)
{
    QMap<QString, QString> settings = SettingsBase::getSettings();

    if (flags == Qt::MatchExactly)
        return settings.values(key);

    QStringList result;

    if (flags == Qt::MatchStartsWith) {
        QStringList keys = settings.keys();

        foreach (const QString &currentKey, keys) {
            if (currentKey.startsWith(key))
                result.append(settings.value(currentKey));
        }
    }

    return result;
}

QStringList LauncherSettings::subKeys(const QString &key)
{
    QMap<QString, QString> settings = SettingsBase::getSettings();
    QStringList keys = settings.uniqueKeys();

    QRegExp keyRe("(.+)/");

    QStringList result;

    foreach (const QString &currentKey, keys) {

        if (keyRe.indexIn(currentKey) != -1) {

            QString prefixedKey = keyRe.cap(1);
            if(prefixedKey.startsWith(key)) {

                QString subKey = prefixedKey.remove(key);
                if (!subKey.isEmpty())
                    result.append(subKey);
            }
        }
    }

    result.removeDuplicates();
    return result;
}

bool LauncherSettings::writeFile(QTextStream &stream)
{
    QString sectionPrefix;
    QRegExp sectionRe("([^/]+)/(.+)$");
    QMap<QString, QString> settings = SettingsBase::getSettings();

    QMapIterator<QString, QString> i(settings);
    i.toBack();

    while (i.hasPrevious()) {
        i.previous();

        QString prefix;
        QString key;

        if (sectionRe.exactMatch(i.key())) {
             prefix = sectionRe.cap(1);
             key = sectionRe.cap(2);
        }

        // Get rid of legacy settings
        if (key.contains(QChar('\\')))
            continue;

        if (key == QLatin1String("CurrentProfile"))
            continue;

        if (sectionPrefix != prefix) {
            sectionPrefix = prefix;
            stream << "\n[" << prefix << "]\n";
        }

        stream << key << "=" << i.value() << "\n";
    }

    return true;

}
