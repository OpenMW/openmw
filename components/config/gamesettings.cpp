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
    template <typename T>
    QList<T> reverse(QList<T> values)
    {
        std::reverse(values.begin(), values.end());
        return values;
    }
}

Config::GameSettings::GameSettings(const Files::ConfigurationManager& cfg)
    : mCfgMgr(cfg)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // this needs calling once so Qt can see its stream operators, which it needs when dragging and dropping
    // it's automatic with Qt 6
    qRegisterMetaTypeStreamOperators<SettingValue>("Config::SettingValue");
#endif
}

void Config::GameSettings::validatePaths()
{
    QList<SettingValue> paths = mSettings.values(QString("data"));

    mDataDirs.clear();

    for (const auto& dataDir : paths)
    {
        if (QDir(dataDir.value).exists())
        {
            SettingValue copy = dataDir;
            copy.value = QDir(dataDir.value).canonicalPath();
            mDataDirs.append(copy);
        }
    }

    // Do the same for data-local
    const QString& local = mSettings.value(QString("data-local")).value;

    if (!local.isEmpty() && QDir(local).exists())
    {
        mDataLocal = QDir(local).canonicalPath();
    }
}

QString Config::GameSettings::getResourcesVfs() const
{
    QString resources = mSettings.value(QString("resources"), { "./resources", "", "" }).value;
    resources += "/vfs";
    return QFileInfo(resources).canonicalFilePath();
}

QList<Config::SettingValue> Config::GameSettings::values(
    const QString& key, const QList<SettingValue>& defaultValues) const
{
    if (!mSettings.values(key).isEmpty())
        return mSettings.values(key);
    return defaultValues;
}

bool Config::GameSettings::containsValue(const QString& key, const QString& value) const
{
    auto [itr, end] = mSettings.equal_range(key);
    while (itr != end)
    {
        if (itr->value == value)
            return true;
        ++itr;
    }
    return false;
}

bool Config::GameSettings::readFile(QTextStream& stream, const QString& context, bool ignoreContent)
{
    if (readFile(stream, mSettings, context, ignoreContent))
    {
        mContexts.push_back(context);
        return true;
    }
    return false;
}

bool Config::GameSettings::readUserFile(QTextStream& stream, const QString& context, bool ignoreContent)
{
    return readFile(stream, mUserSettings, context, ignoreContent);
}

bool Config::GameSettings::readFile(
    QTextStream& stream, QMultiMap<QString, SettingValue>& settings, const QString& context, bool ignoreContent)
{
    QMultiMap<QString, SettingValue> cache;
    QRegularExpression replaceRe("^\\s*replace\\s*=\\s*(.+)$");
    QRegularExpression keyRe("^([^=]+)\\s*=\\s*(.+)$");

    auto initialPos = stream.pos();

    while (!stream.atEnd())
    {
        QString line = stream.readLine();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        QRegularExpressionMatch match = replaceRe.match(line);
        if (match.hasMatch())
        {
            QString key = match.captured(1).trimmed();
            // Replace composing entries with a replace= line
            if (key == QLatin1String("config") || key == QLatin1String("replace") || key == QLatin1String("data")
                || key == QLatin1String("fallback-archive") || key == QLatin1String("content")
                || key == QLatin1String("groundcover") || key == QLatin1String("script-blacklist")
                || key == QLatin1String("fallback"))
                settings.remove(key);
        }
    }

    stream.seek(initialPos);

    while (!stream.atEnd())
    {
        QString line = stream.readLine();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        QRegularExpressionMatch match = keyRe.match(line);
        if (match.hasMatch())
        {
            QString key = match.captured(1).trimmed();
            SettingValue value{ match.captured(2).trimmed(), value.value, context };

            // Don't remove composing entries
            if (key != QLatin1String("config") && key != QLatin1String("replace") && key != QLatin1String("data")
                && key != QLatin1String("fallback-archive") && key != QLatin1String("content")
                && key != QLatin1String("groundcover") && key != QLatin1String("script-blacklist")
                && key != QLatin1String("fallback"))
                settings.remove(key);

            if (key == QLatin1String("config") || key == QLatin1String("user-data") || key == QLatin1String("resources")
                || key == QLatin1String("data") || key == QLatin1String("data-local")
                || key == QLatin1String("load-savegame"))
            {
                // Path line (e.g. 'data=...'), so needs processing to deal with ampersands and quotes
                // The following is based on boost::io::detail::quoted_manip.hpp, but we don't actually use
                // boost::filesystem::path anymore, and use a custom class MaybeQuotedPath which uses Boost-like quoting
                // rules but internally stores as a std::filesystem::path.
                // Caution: This is intentional behaviour to duplicate how Boost and what we replaced it with worked,
                // and we rely on that.
                QChar delim = '\"';
                QChar escape = '&';

                if (value.value.at(0) == delim)
                {
                    QString valueOriginal = value.value;
                    value.value = "";

                    for (QString::const_iterator it = valueOriginal.begin() + 1; it != valueOriginal.end(); ++it)
                    {
                        if (*it == escape)
                            ++it;
                        else if (*it == delim)
                            break;
                        value.value += *it;
                    }
                    value.originalRepresentation = value.value;
                }

                value = procesPathSettingValue(value);
            }
            if (ignoreContent && (key == QLatin1String("content") || key == QLatin1String("data")))
                continue;

            QList<SettingValue> values = cache.values(key);
            values.append(settings.values(key));

            bool exists = false;
            for (const auto& existingValue : values)
            {
                if (existingValue.value == value.value)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
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
        if (i.key() == QLatin1String("config") || i.key() == QLatin1String("user-data")
            || i.key() == QLatin1String("resources") || i.key() == QLatin1String("data")
            || i.key() == QLatin1String("data-local") || i.key() == QLatin1String("load-savegame"))
        {
            stream << i.key() << "=";

            // Equivalent to stream << std::quoted(i.value(), '"', '&'), which won't work on QStrings.
            QChar delim = '\"';
            QChar escape = '&';
            QString string = i.value().originalRepresentation;

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

        stream << i.key() << "=" << i.value().originalRepresentation << "\n";
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
            QMultiMap<QString, SettingValue>::const_iterator i = mUserSettings.find(key);
            while (i != mUserSettings.end() && i.key() == key)
            {
                // todo: does this need to handle paths?
                QString settingLine = i.key() + "=" + i.value().originalRepresentation;
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
        if (it.key() == QLatin1String("config") || it.key() == QLatin1String("user-data")
            || it.key() == QLatin1String("resources") || it.key() == QLatin1String("data")
            || it.key() == QLatin1String("data-local") || it.key() == QLatin1String("load-savegame"))
        {
            settingLine = it.key() + "=";

            // Equivalent to settingLine += std::quoted(it.value(), '"', '&'), which won't work on QStrings.
            QChar delim = '\"';
            QChar escape = '&';
            QString string = it.value().originalRepresentation;

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
            settingLine = it.key() + "=" + it.value().originalRepresentation;

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
    QList<SettingValue> content = mSettings.values(QString(Config::GameSettings::sContentKey));
    for (int i = 0; i < content.count(); ++i)
    {
        if (content.at(i).value.endsWith(QLatin1String(".omwgame"), Qt::CaseInsensitive)
            || content.at(i).value.endsWith(QLatin1String(".esm"), Qt::CaseInsensitive))
        {
            result = true;
            break;
        }
    }

    return result;
}

void Config::GameSettings::setContentList(
    const QList<SettingValue>& dirNames, const QList<SettingValue>& archiveNames, const QStringList& fileNames)
{
    remove(sDirectoryKey);
    for (auto const& item : dirNames)
        setMultiValue(sDirectoryKey, item);
    remove(sArchiveKey);
    for (auto const& item : archiveNames)
        setMultiValue(sArchiveKey, item);
    remove(sContentKey);
    for (auto const& item : fileNames)
        setMultiValue(sContentKey, { item });
}

QList<Config::SettingValue> Config::GameSettings::getDataDirs() const
{
    return reverse(mDataDirs);
}

QList<Config::SettingValue> Config::GameSettings::getArchiveList() const
{
    // QMap returns multiple rows in LIFO order, so need to reverse
    return reverse(values(sArchiveKey));
}

QList<Config::SettingValue> Config::GameSettings::getContentList() const
{
    // QMap returns multiple rows in LIFO order, so need to reverse
    return reverse(values(sContentKey));
}

bool Config::GameSettings::isUserSetting(const SettingValue& settingValue) const
{
    return settingValue.context.isEmpty() || settingValue.context == getUserContext();
}

Config::SettingValue Config::GameSettings::procesPathSettingValue(const SettingValue& value)
{
    std::filesystem::path path = Files::pathFromQString(value.value);
    std::filesystem::path basePath = Files::pathFromQString(value.context.isEmpty() ? getUserContext() : value.context);
    mCfgMgr.processPath(path, basePath);
    return SettingValue{ Files::pathToQString(path), value.originalRepresentation, value.context };
}

void Config::GameSettings::clear()
{
    mSettings.clear();
    mContexts.clear();
    mUserSettings.clear();
    mDataDirs.clear();
    mDataLocal.clear();
}

QDataStream& Config::operator<<(QDataStream& out, const SettingValue& settingValue)
{
    out << settingValue.value;
    out << settingValue.originalRepresentation;
    out << settingValue.context;
    return out;
}

QDataStream& Config::operator>>(QDataStream& in, SettingValue& settingValue)
{
    in >> settingValue.value;
    in >> settingValue.originalRepresentation;
    in >> settingValue.context;
    return in;
}
