#include "gamesettings.hpp"
#include "launchersettings.hpp"

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

const char Config::GameSettings::sContentKey[] = "content";

Config::GameSettings::GameSettings(Files::ConfigurationManager &cfg)
    : mCfgMgr(cfg)
{
}

Config::GameSettings::~GameSettings()
{
}

void Config::GameSettings::validatePaths()
{
    QStringList paths = mSettings.values(QString("data"));
    Files::PathContainer dataDirs;

    foreach (const QString &path, paths) {
        QByteArray bytes = path.toUtf8();
        dataDirs.push_back(Files::PathContainer::value_type(std::string(bytes.constData(), bytes.length())));
    }

    // Parse the data dirs to convert the tokenized paths
    mCfgMgr.processPaths(dataDirs);
    mDataDirs.clear();

    for (Files::PathContainer::iterator it = dataDirs.begin(); it != dataDirs.end(); ++it) {
        QString path = QString::fromUtf8(it->string().c_str());
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
    QByteArray bytes = local.toUtf8();
    dataDirs.push_back(Files::PathContainer::value_type(std::string(bytes.constData(), bytes.length())));

    mCfgMgr.processPaths(dataDirs);

    if (!dataDirs.empty()) {
        QString path = QString::fromUtf8(dataDirs.front().string().c_str());
        path.remove(QChar('\"'));

        QDir dir(path);
        if (dir.exists())
            mDataLocal = path;
    }
}

QStringList Config::GameSettings::values(const QString &key, const QStringList &defaultValues) const
{
    if (!mSettings.values(key).isEmpty())
        return mSettings.values(key);
    return defaultValues;
}

bool Config::GameSettings::readFile(QTextStream &stream)
{
    return readFile(stream, mSettings);
}

bool Config::GameSettings::readUserFile(QTextStream &stream)
{
    return readFile(stream, mUserSettings);
}

bool Config::GameSettings::readFile(QTextStream &stream, QMap<QString, QString> &settings)
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

bool Config::GameSettings::writeFile(QTextStream &stream)
{
    // Iterate in reverse order to preserve insertion order
    QMapIterator<QString, QString> i(mUserSettings);
    i.toBack();

    while (i.hasPrevious()) {
        i.previous();

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

    return true;
}

bool Config::GameSettings::hasMaster()
{
    bool result = false;
    QStringList content = mSettings.values(QString(Config::GameSettings::sContentKey));
    for (int i = 0; i < content.count(); ++i) {
        if (content.at(i).contains(".omwgame") || content.at(i).contains(".esm")) {
            result = true;
            break;
        }
    }

    return result;
}

void Config::GameSettings::setContentList(const QStringList& fileNames)
{
    remove(sContentKey);
    foreach(const QString& fileName, fileNames)
    {
        setMultiValue(sContentKey, fileName);
    }
}

QStringList Config::GameSettings::getContentList() const
{
    // QMap returns multiple rows in LIFO order, so need to reverse
    return Config::LauncherSettings::reverse(values(sContentKey));
}

