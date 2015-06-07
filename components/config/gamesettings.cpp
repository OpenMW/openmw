#include "gamesettings.hpp"
#include "launchersettings.hpp"

#include <QTextCodec>
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

// Policy:
//
// - Always ignore a line beginning with '#' or empty lines
//
// - If a line in file exists with matching key and first part of value (before ',',
//   '\n', etc) also matches, then replace the line with that of mUserSettings.
// - else remove line (TODO: maybe replace the line with '#' in front instead?)
//
// - If there is no corresponding line in file, add at the end
//
bool Config::GameSettings::writeFileWithComments(QFile &file)
{
    QTextStream stream(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    // slurp
    std::vector<QString> fileCopy;
    QString line = stream.readLine();
    while (!line.isNull())
    {
        fileCopy.push_back(line);
        line = stream.readLine();
    }
    stream.seek(0);

    // empty file, no comments to keep
    if (fileCopy.empty())
        return writeFile(stream);

    // Temp copy of settings to save, but with the keys appended with the first part of the value
    //
    // ATTENTION!
    //
    // A hack to avoid looping through each line, makes use of the fact that fallbacks values
    // are comma separated.
    QMap<QString, QString> userSettingsCopy;
    QRegExp settingRegex("^([^=]+)\\s*=\\s*([^,]+)(.*)$");
    QString settingLine;
    QMap<QString, QString>::const_iterator settingsIter = mUserSettings.begin();
    for (; settingsIter != mUserSettings.end(); ++settingsIter)
    {
        settingLine = settingsIter.key()+"="+settingsIter.value();
        if (settingRegex.indexIn(settingLine) != -1)
        {
            userSettingsCopy[settingRegex.cap(1)+"="+settingRegex.cap(2)] =
                (settingRegex.captureCount() < 3) ? "" : settingRegex.cap(3);
        }
    }

    QString keyVal;
    for (std::vector<QString>::iterator iter = fileCopy.begin(); iter != fileCopy.end(); ++iter)
    {
        // skip empty or comment lines
        if ((*iter).isEmpty() || (*iter).contains(QRegExp("^\\s*#")))
            continue;

        // look for a key in the line
        if (settingRegex.indexIn(*iter) == -1 || settingRegex.captureCount() < 2)
        {
            // no key or first part of value found in line, replace with a null string which
            // will be remved later
            *iter = QString();
            continue;
        }

        // look for a matching key in user settings
        keyVal = settingRegex.cap(1)+"="+settingRegex.cap(2);
        QMap<QString, QString>::iterator it = userSettingsCopy.find(keyVal);
        if (it == userSettingsCopy.end())
        {
            // no such key+valStart, replace with a null string which will be remved later
            *iter = QString();
        }
        else
        {
            *iter = QString(it.key()+it.value());
            userSettingsCopy.erase(it);
        }
    }

    // write the new config file
    QString key;
    QString value;
    for (std::vector<QString>::iterator iter = fileCopy.begin(); iter != fileCopy.end(); ++iter)
    {
        if ((*iter).isNull())
            continue;

        // Below is based on readFile() code, if that changes corresponding change may be
        // required (for example duplicates may be inserted if the rules don't match)
        if ((*iter).isEmpty() || (*iter).contains(QRegExp("^\\s*#")))
        {
            stream << *iter << "\n";
            continue;
        }

        if (settingRegex.indexIn(*iter) == -1 || settingRegex.captureCount() < 2)
            continue;

        // Quote paths with spaces
        key = settingRegex.cap(1);
        value = settingRegex.cap(2)+settingRegex.cap(3);
        if (key == QLatin1String("data")
            || key == QLatin1String("data-local")
            || key == QLatin1String("resources"))
        {
            if (value.contains(QChar(' ')))
            {
                value.remove(QChar('\"')); // Remove quotes

                stream << key << "=\"" << value << "\"\n";
                continue;
            }
        }
        stream << key << "=" << value << "\n";
    }

    if (!userSettingsCopy.empty())
    {
        stream << "# new entries" << "\n";
        QMap<QString, QString>::const_iterator it = userSettingsCopy.begin();
        for (; it != userSettingsCopy.end(); ++it)
        {
            stream << it.key() << it.value() << "\n";
        }
    }

    file.resize(file.pos());

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

