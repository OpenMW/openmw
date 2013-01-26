#include <QTextStream>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QMap>

#include <QDebug>

#include <components/files/configurationmanager.hpp>

#include "gamesettings.hpp"

GameSettings::GameSettings(Files::ConfigurationManager &cfg)
    : mCfgMgr(cfg)
{
}

GameSettings::~GameSettings()
{
}

void GameSettings::validatePaths()
{
    qDebug() << "validate paths!";

    if (mSettings.isEmpty())
        return;

    QStringList paths = mSettings.values(QString("data"));
    Files::PathContainer dataDirs;

    foreach (const QString &path, paths) {
        dataDirs.push_back(Files::PathContainer::value_type(path.toStdString()));
    }

    // Parse the data dirs to convert the tokenized paths
    mCfgMgr.processPaths(dataDirs);

//    // Replace the existing data paths with valid untokenized ones
//    mSettings.remove(QString("data"));
    mDataDirs.clear();

    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it) {
        QString path = QString::fromStdString(it->string());
        path.remove(QChar('\"'));

        QDir dir(path);
        if (dir.exists())
            mDataDirs.append(path);
    }

    // Do the same for data-local
    QString local = mSettings.value(QString("data-local"));

    if (local.isEmpty())
        return;

    dataDirs.clear();
    dataDirs.push_back(Files::PathContainer::value_type(local.toStdString()));

    mCfgMgr.processPaths(dataDirs);
//    mSettings.remove(QString("data-local"));

    if (!dataDirs.empty()) {
        QString path = QString::fromStdString(dataDirs.front().string());
        path.remove(QChar('\"'));

        QDir dir(path);
        if (dir.exists())
            mDataLocal = path;
    }
    qDebug() << mSettings;


}

QStringList GameSettings::values(const QString &key, const QStringList &defaultValues)
{
    if (!mSettings.values(key).isEmpty())
        return mSettings.values(key);
    return defaultValues;
}

bool GameSettings::readFile(QTextStream &stream)
{
    QMap<QString, QString> cache;
    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    while (!stream.atEnd()) {
        QString line = stream.readLine().simplified();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        qDebug() << "line: " << line;
        if (keyRe.indexIn(line) != -1) {

            QString key = keyRe.cap(1).simplified();
            QString value = keyRe.cap(2).simplified();

            qDebug() << "key: " << key;
            // There can be multiple data keys
            if (key == QLatin1String("data")) {
                cache.insertMulti(key, value);
            } else {
                cache.insert(key, value);
            }
        }
    }

    if (mSettings.isEmpty()) {
        mSettings = cache; // This is the first time we read a file
        validatePaths();
        return true;
    }

    // Replace values from previous settings
    QMapIterator<QString, QString> i(cache);
    while (i.hasNext()) {
        i.next();

        // Don't remove existing data entries
        if (i.key() == QLatin1String("data"))
            continue;

        if (mSettings.contains(i.key()))
            mSettings.remove(i.key());
    }

    // Merge the changed keys with those which didn't
    mSettings.unite(cache);
    validatePaths();
    qDebug() << mSettings;
    return true;
}

bool GameSettings::writeFile(QTextStream &stream)
{
    QMapIterator<QString, QString> i(mSettings);
    while (i.hasNext()) {
        i.next();

        // Quote values with spaces
        if (i.value().contains(" ")) {
            stream << i.key() << "=\"" << i.value() << "\"\n";
        } else {
            stream << i.key() << "=" << i.value() << "\n";
        }

    }
}
