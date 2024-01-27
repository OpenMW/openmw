#include "gamesettings.hpp"

#include <QDir>
#include <QRegularExpression>

#include <components/files/configurationmanager.hpp>
#include <components/files/qtconversion.hpp>
#include <components/misc/utf8qtextstream.hpp>

const char Config::GameSettings::sArchiveKey[] = "fallback-archive";
const char Config::GameSettings::sContentKey[] = "content";
const char Config::GameSettings::sDirectoryKey[] = "data";

namespace
{
    QStringList reverse(QStringList values)
    {
        std::reverse(values.begin(), values.end());
        return values;
    }
}

Config::GameSettings::GameSettings(const Files::ConfigurationManager& cfg)
    : mCfgMgr(cfg)
{
}

void Config::GameSettings::validatePaths()
{
    QStringList paths = mSettings.values(QString("data"));
    Files::PathContainer dataDirs;

    for (const QString& path : paths)
    {
        dataDirs.emplace_back(Files::pathFromQString(path));
    }

    // Parse the data dirs to convert the tokenized paths
    mCfgMgr.processPaths(dataDirs, /*basePath=*/"");
    mDataDirs.clear();

    for (const auto& dataDir : dataDirs)
    {
        if (is_directory(dataDir))
            mDataDirs.append(Files::pathToQString(dataDir));
    }

    // Do the same for data-local
    QString local = mSettings.value(QString("data-local"));
    if (local.length() && local.at(0) == QChar('\"'))
    {
        local.remove(0, 1);
        local.chop(1);
    }

    if (local.isEmpty())
        return;

    dataDirs.clear();
    dataDirs.emplace_back(Files::pathFromQString(local));

    mCfgMgr.processPaths(dataDirs, /*basePath=*/"");

    if (!dataDirs.empty())
    {
        const auto& path = dataDirs.front();
        if (is_directory(path))
            mDataLocal = Files::pathToQString(path);
    }
}

std::filesystem::path Config::GameSettings::getGlobalDataDir() const
{
    // global data dir may not exists if OpenMW is not installed (ie if run from build directory)
    const auto& path = mCfgMgr.getGlobalDataPath();
    if (std::filesystem::exists(path))
        return std::filesystem::canonical(path);
    return {};
}

QStringList Config::GameSettings::values(const QString& key, const QStringList& defaultValues) const
{
    if (!mSettings.values(key).isEmpty())
        return mSettings.values(key);
    return defaultValues;
}

bool Config::GameSettings::readFile(QTextStream& stream, bool ignoreContent)
{
    return readFile(stream, mSettings, ignoreContent);
}

bool Config::GameSettings::readUserFile(QTextStream& stream, bool ignoreContent)
{
    return readFile(stream, mUserSettings, ignoreContent);
}

bool Config::GameSettings::readFile(QTextStream& stream, QMultiMap<QString, QString>& settings, bool ignoreContent)
{
    QMultiMap<QString, QString> cache;
    QRegularExpression keyRe("^([^=]+)\\s*=\\s*(.+)$");

    while (!stream.atEnd())
    {
        QString line = stream.readLine();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        QRegularExpressionMatch match = keyRe.match(line);
        if (match.hasMatch())
        {
            QString key = match.captured(1).trimmed();
            QString value = match.captured(2).trimmed();

            // Don't remove composing entries
            if (key != QLatin1String("data") && key != QLatin1String("fallback-archive")
                && key != QLatin1String("content") && key != QLatin1String("groundcover")
                && key != QLatin1String("script-blacklist"))
                settings.remove(key);

            if (key == QLatin1String("data") || key == QLatin1String("data-local") || key == QLatin1String("resources")
                || key == QLatin1String("load-savegame"))
            {
                // Path line (e.g. 'data=...'), so needs processing to deal with ampersands and quotes
                // The following is based on boost::io::detail::quoted_manip.hpp, but calling those functions did not
                // work as there are too may QStrings involved
                QChar delim = '\"';
                QChar escape = '&';

                if (value.at(0) == delim)
                {
                    QString valueOriginal = value;
                    value = "";

                    for (QString::const_iterator it = valueOriginal.begin() + 1; it != valueOriginal.end(); ++it)
                    {
                        if (*it == escape)
                            ++it;
                        else if (*it == delim)
                            break;
                        value += *it;
                    }
                }
            }
            if (ignoreContent && (key == QLatin1String("content") || key == QLatin1String("data")))
                continue;

            QStringList values = cache.values(key);
            values.append(settings.values(key));

            if (!values.contains(value))
            {
                cache.insert(key, value);
            }
        }
    }

    if (settings.isEmpty())
    {
        settings = cache; // This is the first time we read a file
        validatePaths();
        return true;
    }

    // Merge the changed keys with those which didn't
    settings.unite(cache);
    validatePaths();

    return true;
}

bool Config::GameSettings::writeFile(QTextStream& stream)
{
    // Iterate in reverse order to preserve insertion order
    auto i = mUserSettings.end();
    while (i != mUserSettings.begin())
    {
        i--;

        // path lines (e.g. 'data=...') need quotes and ampersands escaping to match how boost::filesystem::path uses
        // boost::io::quoted
        // We don't actually use boost::filesystem::path anymore, but use a custom class MaybeQuotedPath which uses
        // Boost-like quoting rules but internally stores as a std::filesystem::path.
        // Caution: This is intentional behaviour to duplicate how Boost and what we replaced it with worked, and we
        // rely on that.
        if (i.key() == QLatin1String("data") || i.key() == QLatin1String("data-local")
            || i.key() == QLatin1String("resources") || i.key() == QLatin1String("load-savegame"))
        {
            stream << i.key() << "=";

            // Equivalent to stream << std::quoted(i.value(), '"', '&'), which won't work on QStrings.
            QChar delim = '\"';
            QChar escape = '&';
            QString string = i.value();

            stream << delim;
            for (auto& it : string)
            {
                if (it == delim || it == escape)
                    stream << escape;
                stream << it;
            }
            stream << delim;

            stream << '\n';
            continue;
        }

        stream << i.key() << "=" << i.value() << "\n";
    }

    return true;
}

bool Config::GameSettings::isOrderedLine(const QString& line)
{
    return line.contains(QRegularExpression("^\\s*fallback-archive\\s*="))
        || line.contains(QRegularExpression("^\\s*fallback\\s*="))
        || line.contains(QRegularExpression("^\\s*data\\s*="))
        || line.contains(QRegularExpression("^\\s*data-local\\s*="))
        || line.contains(QRegularExpression("^\\s*resources\\s*="))
        || line.contains(QRegularExpression("^\\s*groundcover\\s*="))
        || line.contains(QRegularExpression("^\\s*content\\s*="));
}

// Policy:
//
// - Always ignore a line beginning with '#' or empty lines; added above a config
//   entry.
//
// - If a line in file exists with matching key and first part of value (before ',',
//   '\n', etc) also matches, then replace the line with that of mUserSettings.
// - else remove line
//
// - If there is no corresponding line in file, add at the end
//
// - Removed content items are saved as comments if the item had any comments.
//   Content items prepended with '##' are considered previously removed.
//
bool Config::GameSettings::writeFileWithComments(QFile& file)
{
    QTextStream stream(&file);
    Misc::ensureUtf8Encoding(stream);

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

    // start
    //   |
    //   |    +----------------------------------------------------------+
    //   |    |                                                          |
    //   v    v                                                          |
    // skip non-"ordered" lines (remove "ordered" lines)                 |
    //   |              ^                                                |
    //   |              |                                                |
    //   |      non-"ordered" line, write saved comments                 |
    //   |              ^                                                |
    //   v              |                                                |
    // blank or comment line, save in temp buffer <--------+             |
    //        |                |                           |             |
    //        |                +------- comment line ------+             |
    //        v                    (special processing '##')             |
    //  "ordered" line                                                   |
    //        |                                                          |
    //        v                                                          |
    // save in a separate map of comments keyed by "ordered" line        |
    //        |                                                          |
    //        +----------------------------------------------------------+
    //
    //
    QRegularExpression settingRegex("^([^=]+)\\s*=\\s*([^,]+)(.*)$");
    std::vector<QString> comments;
    auto commentStart = fileCopy.end();
    std::map<QString, std::vector<QString>> commentsMap;
    for (auto iter = fileCopy.begin(); iter != fileCopy.end(); ++iter)
    {
        if (isOrderedLine(*iter))
        {
            // save in a separate map of comments keyed by "ordered" line
            if (!comments.empty())
            {
                QRegularExpressionMatch match = settingRegex.match(*iter);
                if (match.hasMatch())
                {
                    commentsMap[match.captured(1) + "=" + match.captured(2)] = comments;
                    comments.clear();
                    commentStart = fileCopy.end();
                }
                // else do nothing, malformed line
            }

            *iter = QString(); // "ordered" lines to be removed later
        }
        else if ((*iter).isEmpty() || (*iter).contains(QRegularExpression("^\\s*#")))
        {
            // comment line, save in temp buffer
            if (comments.empty())
                commentStart = iter;

            // special removed content processing
            if ((*iter).contains(QRegularExpression("^##content\\s*=")))
            {
                if (!comments.empty())
                {
                    commentsMap[*iter] = comments;
                    comments.clear();
                    commentStart = fileCopy.end();
                }
            }
            else
                comments.push_back(*iter);

            *iter = QString(); // assume to be deleted later
        }
        else
        {
            QRegularExpressionMatch match = settingRegex.match(*iter);

            // blank or non-"ordered" line, write saved comments
            if (!comments.empty() && match.hasMatch() && settingRegex.captureCount() >= 2
                && mUserSettings.find(match.captured(1)) != mUserSettings.end())
            {
                if (commentStart == fileCopy.end())
                    throw std::runtime_error(
                        "Config::GameSettings: failed to parse settings - iterator is past of end of settings file");

                for (const auto& comment : comments)
                {
                    *commentStart = comment;
                    ++commentStart;
                }
                comments.clear();
                commentStart = fileCopy.end();
            }

            // keep blank lines and non-"ordered" lines other than comments

            // look for a key in the line
            if (!match.hasMatch() || settingRegex.captureCount() < 2)
            {
                // no key or first part of value found in line, replace with a null string which
                // will be removed later
                *iter = QString();
                comments.clear();
                commentStart = fileCopy.end();
                continue;
            }

            // look for a matching key in user settings
            *iter = QString(); // assume no match
            QString key = match.captured(1);
            QString keyVal = match.captured(1) + "=" + match.captured(2);
            QMultiMap<QString, QString>::const_iterator i = mUserSettings.find(key);
            while (i != mUserSettings.end() && i.key() == key)
            {
                QString settingLine = i.key() + "=" + i.value();
                QRegularExpressionMatch keyMatch = settingRegex.match(settingLine);
                if (keyMatch.hasMatch())
                {
                    if ((keyMatch.captured(1) + "=" + keyMatch.captured(2)) == keyVal)
                    {
                        *iter = std::move(settingLine);
                        break;
                    }
                }
                ++i;
            }
        }
    }

    // comments at top of file
    for (auto& iter : fileCopy)
    {
        if (iter.isNull())
            continue;

        // Below is based on readFile() code, if that changes corresponding change may be
        // required (for example duplicates may be inserted if the rules don't match)
        if (/*(*iter).isEmpty() ||*/ iter.contains(QRegularExpression("^\\s*#")))
        {
            stream << iter << "\n";
            continue;
        }
    }

    // Iterate in reverse order to preserve insertion order
    QString settingLine;
    auto it = mUserSettings.end();
    while (it != mUserSettings.begin())
    {
        it--;

        // path lines (e.g. 'data=...') need quotes and ampersands escaping to match how boost::filesystem::path uses
        // boost::io::quoted
        // We don't actually use boost::filesystem::path anymore, but use a custom class MaybeQuotedPath which uses
        // Boost-like quoting rules but internally stores as a std::filesystem::path.
        // Caution: This is intentional behaviour to duplicate how Boost and what we replaced it with worked, and we
        // rely on that.
        if (it.key() == QLatin1String("data") || it.key() == QLatin1String("data-local")
            || it.key() == QLatin1String("resources") || it.key() == QLatin1String("load-savegame"))
        {
            settingLine = it.key() + "=";

            // Equivalent to settingLine += std::quoted(it.value(), '"', '&'), which won't work on QStrings.
            QChar delim = '\"';
            QChar escape = '&';
            QString string = it.value();

            settingLine += delim;
            for (auto& iter : string)
            {
                if (iter == delim || iter == escape)
                    settingLine += escape;
                settingLine += iter;
            }
            settingLine += delim;
        }
        else
            settingLine = it.key() + "=" + it.value();

        QRegularExpressionMatch match = settingRegex.match(settingLine);
        if (match.hasMatch())
        {
            auto i = commentsMap.find(match.captured(1) + "=" + match.captured(2));

            // check if previous removed content item with comments
            if (i == commentsMap.end())
                i = commentsMap.find("##" + match.captured(1) + "=" + match.captured(2));

            if (i != commentsMap.end())
            {
                std::vector<QString> cLines = i->second;
                for (const auto& cLine : cLines)
                    stream << cLine << "\n";

                commentsMap.erase(i);
            }
        }

        stream << settingLine << "\n";
    }

    // flush any removed settings
    if (!commentsMap.empty())
    {
        auto i = commentsMap.begin();
        for (; i != commentsMap.end(); ++i)
        {
            if (i->first.contains(QRegularExpression("^\\s*content\\s*=")))
            {
                std::vector<QString> cLines = i->second;
                for (const auto& cLine : cLines)
                    stream << cLine << "\n";

                // mark the content line entry for future preocessing
                stream << "##" << i->first << "\n";

                // commentsMap.erase(i);
            }
        }
    }

    // flush any end comments
    if (!comments.empty())
    {
        for (const auto& comment : comments)
            stream << comment << "\n";
    }

    file.resize(file.pos());

    return true;
}

bool Config::GameSettings::hasMaster()
{
    bool result = false;
    QStringList content = mSettings.values(QString(Config::GameSettings::sContentKey));
    for (int i = 0; i < content.count(); ++i)
    {
        if (content.at(i).endsWith(QLatin1String(".omwgame"), Qt::CaseInsensitive)
            || content.at(i).endsWith(QLatin1String(".esm"), Qt::CaseInsensitive))
        {
            result = true;
            break;
        }
    }

    return result;
}

void Config::GameSettings::setContentList(
    const QStringList& dirNames, const QStringList& archiveNames, const QStringList& fileNames)
{
    auto const reset = [this](const char* key, const QStringList& list) {
        remove(key);
        for (auto const& item : list)
            setMultiValue(key, item);
    };

    reset(sDirectoryKey, dirNames);
    reset(sArchiveKey, archiveNames);
    reset(sContentKey, fileNames);
}

QStringList Config::GameSettings::getDataDirs() const
{
    return reverse(mDataDirs);
}

QStringList Config::GameSettings::getArchiveList() const
{
    // QMap returns multiple rows in LIFO order, so need to reverse
    return reverse(values(sArchiveKey));
}

QStringList Config::GameSettings::getContentList() const
{
    // QMap returns multiple rows in LIFO order, so need to reverse
    return reverse(values(sContentKey));
}

void Config::GameSettings::clear()
{
    mSettings.clear();
    mUserSettings.clear();
    mDataDirs.clear();
    mDataLocal.clear();
}
