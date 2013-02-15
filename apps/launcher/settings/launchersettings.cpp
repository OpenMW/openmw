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

QStringList LauncherSettings::values(const QString &key, Qt::MatchFlags flags)
{
    QMap<QString, QString> settings = SettingsBase::getSettings();

    if (flags == Qt::MatchExactly)
        return settings.values(key);

    QStringList result;

    if (flags == Qt::MatchStartsWith) {
        QStringList keys = settings.keys();

        foreach (const QString &currentKey, keys) {
            qDebug() << "key is: " << currentKey << "value: " << settings.value(currentKey);
            if (currentKey.startsWith(key))
                result.append(settings.value(currentKey));
        }
    }

    return result;
}

QStringList LauncherSettings::subKeys(const QString &key)
{
    QMap<QString, QString> settings = SettingsBase::getSettings();
    QStringList keys = settings.keys();

    QRegExp keyRe("(.+)/");

    QStringList result;

    foreach (const QString &currentKey, keys) {
        qDebug() << "key is: " << currentKey;
        if (keyRe.indexIn(currentKey) != -1) {
            qDebug() << "text: " <<  keyRe.cap(1) << keyRe.cap(2);

            QString prefixedKey = keyRe.cap(1);
            if(prefixedKey.startsWith(key)) {

                QString subKey = prefixedKey.remove(key);
                if (!subKey.isEmpty())
                    result.append(subKey);
                //qDebug() <<  keyRe.cap(2).simplified();
            }
        } else {
            qDebug() << "no match";
        }
    }

    result.removeDuplicates();
    qDebug() << result;

    return result;
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
