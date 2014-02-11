#include "gamesettings.hpp"

#include <QTextStream>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QMap>

#include <components/files/configurationmanager.hpp>

#include <boost/version.hpp>

/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

    template<>
    inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
    {
        return boost::filesystem::path(arg);
    }

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */


Launcher::GameSettings::GameSettings(Files::ConfigurationManager &cfg)
    : mCfgMgr(cfg)
{
}

Launcher::GameSettings::~GameSettings()
{
}

void Launcher::GameSettings::validatePaths()
{
    if (mSettings.isEmpty() || !mDataDirs.isEmpty())
        return; // Don't re-validate paths if they are already parsed

    QStringList paths = mSettings.values(QString("data"));
    Files::PathContainer dataDirs;

    foreach (const QString &path, paths) {
        dataDirs.push_back(Files::PathContainer::value_type(path.toStdString()));
    }

    // Parse the data dirs to convert the tokenized paths
    mCfgMgr.processPaths(dataDirs);
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

    if (!dataDirs.empty()) {
        QString path = QString::fromStdString(dataDirs.front().string());
        path.remove(QChar('\"'));

        QDir dir(path);
        if (dir.exists())
            mDataLocal = path;
    }
}

QStringList Launcher::GameSettings::values(const QString &key, const QStringList &defaultValues)
{
    if (!mSettings.values(key).isEmpty())
        return mSettings.values(key);
    return defaultValues;
}

bool Launcher::GameSettings::readFile(QTextStream &stream)
{
    return readFile(stream, mSettings);
}

bool Launcher::GameSettings::readUserFile(QTextStream &stream)
{
    return readFile(stream, mUserSettings);
}

bool Launcher::GameSettings::readFile(QTextStream &stream, QMap<QString, QString> &settings)
{
    QMap<QString, QString> cache;
    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        if (keyRe.indexIn(line) != -1) {

            QString key = keyRe.cap(1).trimmed();
            QString value = keyRe.cap(2).trimmed();

            // Don't remove existing data entries
            if (key != QLatin1String("data"))
                settings.remove(key);

            QStringList values = cache.values(key);
            values.append(settings.values(key));

            if (!values.contains(value)) {
                cache.insertMulti(key, value);
            }
        }
    }

    if (settings.isEmpty()) {
        settings = cache; // This is the first time we read a file
        validatePaths();
        return true;
    }

    // Merge the changed keys with those which didn't
    settings.unite(cache);
    validatePaths();

    return true;
}


bool Launcher::GameSettings::writeFile(QTextStream &stream)
{
    // Iterate in reverse order to preserve insertion order
    QMapIterator<QString, QString> i(mUserSettings);
    i.toBack();

    while (i.hasPrevious()) {
        i.previous();

        if (i.key() == QLatin1String("content"))
            continue;

        // Quote paths with spaces
        if (i.key() == QLatin1String("data")
            || i.key() == QLatin1String("data-local")
            || i.key() == QLatin1String("resources"))
        {
            if (i.value().contains(QChar(' ')))
            {
                QString stripped = i.value();
                stripped.remove(QChar('\"')); // Remove quotes

                stream << i.key() << "=\"" << stripped << "\"\n";
                continue;
            }
        }

        stream << i.key() << "=" << i.value() << "\n";

    }

    QStringList content = mUserSettings.values(QString("content"));
    for (int i = content.count(); i--;) {
        stream << "content=" << content.at(i) << "\n";
    }

    return true;
}

bool Launcher::GameSettings::hasMaster()
{
    bool result = false;
    QStringList content = mSettings.values(QString("content"));
    for (int i = 0; i < content.count(); ++i) {
        if (content.at(i).contains(".omwgame") || content.at(i).contains(".esm")) {
            result = true;
            break;
        }
    }

    return result;
}
