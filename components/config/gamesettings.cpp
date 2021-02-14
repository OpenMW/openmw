#include "gamesettings.hpp"
#include "launchersettings.hpp"

#include <QTextCodec>
#include <QDir>
#include <QRegExp>

#include <components/files/configurationmanager.hpp>

const char Config::GameSettings::sContentKey[] = "content";

Config::GameSettings::GameSettings(Files::ConfigurationManager &cfg)
    : mCfgMgr(cfg)
{
}

void Config::GameSettings::validatePaths()
{
    QStringList paths = mSettings.values(QString("data"));
    Files::PathContainer dataDirs;

    for (const QString &path : paths)
    {
        QByteArray bytes = path.toUtf8();
        dataDirs.push_back(Files::PathContainer::value_type(std::string(bytes.constData(), bytes.length())));
    }

    // Parse the data dirs to convert the tokenized paths
    mCfgMgr.processPaths(dataDirs);
    mDataDirs.clear();

    for (auto & dataDir : dataDirs) {
        QString path = QString::fromUtf8(dataDir.string().c_str());

        QDir dir(path);
        if (dir.exists())
            mDataDirs.append(path);
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
    QByteArray bytes = local.toUtf8();
    dataDirs.push_back(Files::PathContainer::value_type(std::string(bytes.constData(), bytes.length())));

    mCfgMgr.processPaths(dataDirs);

    if (!dataDirs.empty()) {
        QString path = QString::fromUtf8(dataDirs.front().string().c_str());

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

bool Config::GameSettings::readFile(QTextStream &stream, QMultiMap<QString, QString> &settings)
{
    QMultiMap<QString, QString> cache;
    QRegExp keyRe("^([^=]+)\\s*=\\s*(.+)$");

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        if (keyRe.indexIn(line) != -1) {

            QString key = keyRe.cap(1).trimmed();
            QString value = keyRe.cap(2).trimmed();

            // Don't remove composing entries
            if (key != QLatin1String("data")
                && key != QLatin1String("fallback-archive")
                && key != QLatin1String("content")
                && key != QLatin1String("groundcover")
                && key != QLatin1String("script-blacklist"))
                settings.remove(key);

            if (key == QLatin1String("data")
                || key == QLatin1String("data-local")
                || key == QLatin1String("resources")
                || key == QLatin1String("load-savegame"))
            {
                // Path line (e.g. 'data=...'), so needs processing to deal with ampersands and quotes
                // The following is based on boost::io::detail::quoted_manip.hpp, but calling those functions did not work as there are too may QStrings involved
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

            QStringList values = cache.values(key);
            values.append(settings.values(key));

            if (!values.contains(value)) {
                cache.insert(key, value);
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

        // path lines (e.g. 'data=...') need quotes and ampersands escaping to match how boost::filesystem::path uses boost::io::quoted
        if (i.key() == QLatin1String("data")
            || i.key() == QLatin1String("data-local")
            || i.key() == QLatin1String("resources")
            || i.key() == QLatin1String("load-savegame"))
        {
            stream << i.key() << "=";

            // The following is based on boost::io::detail::quoted_manip.hpp, but calling those functions did not work as there are too may QStrings involved
            QChar delim = '\"';
            QChar escape = '&';
            QString string = i.value();

            stream << delim;
            for (auto it : string)
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
    return line.contains(QRegExp("^\\s*fallback-archive\\s*="))
           || line.contains(QRegExp("^\\s*fallback\\s*="))
           || line.contains(QRegExp("^\\s*data\\s*="))
           || line.contains(QRegExp("^\\s*data-local\\s*="))
           || line.contains(QRegExp("^\\s*resources\\s*="))
           || line.contains(QRegExp("^\\s*groundcover\\s*="))
           || line.contains(QRegExp("^\\s*content\\s*="));
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
    QRegExp settingRegex("^([^=]+)\\s*=\\s*([^,]+)(.*)$");
    std::vector<QString> comments;
    auto commentStart = fileCopy.end();
    std::map<QString, std::vector<QString> > commentsMap;
    for (auto iter = fileCopy.begin(); iter != fileCopy.end(); ++iter)
    {
        if (isOrderedLine(*iter))
        {
            // save in a separate map of comments keyed by "ordered" line
            if (!comments.empty())
            {
                if (settingRegex.indexIn(*iter) != -1)
                {
                    commentsMap[settingRegex.cap(1)+"="+settingRegex.cap(2)] = comments;
                    comments.clear();
                    commentStart = fileCopy.end();
                }
                // else do nothing, malformed line
            }

            *iter = QString(); // "ordered" lines to be removed later
        }
        else if ((*iter).isEmpty() || (*iter).contains(QRegExp("^\\s*#")))
        {
            // comment line, save in temp buffer
            if (comments.empty())
                commentStart = iter;

            // special removed content processing
            if ((*iter).contains(QRegExp("^##content\\s*=")))
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
            int index = settingRegex.indexIn(*iter);

            // blank or non-"ordered" line, write saved comments
            if (!comments.empty() && index != -1 && settingRegex.captureCount() >= 2 &&
                mUserSettings.find(settingRegex.cap(1)) != mUserSettings.end())
            {
                if (commentStart == fileCopy.end())
                    throw std::runtime_error("Config::GameSettings: failed to parse settings - iterator is past of end of settings file");

                for (const auto & comment : comments)
                {
                    *commentStart = comment;
                    ++commentStart;
                }
                comments.clear();
                commentStart = fileCopy.end();
            }

            // keep blank lines and non-"ordered" lines other than comments

            // look for a key in the line
            if (index == -1 || settingRegex.captureCount() < 2)
            {
                // no key or first part of value found in line, replace with a null string which
                // will be remved later
                *iter = QString();
                comments.clear();
                commentStart = fileCopy.end();
                continue;
            }

            // look for a matching key in user settings
            *iter = QString(); // assume no match
            QString key = settingRegex.cap(1);
            QString keyVal = settingRegex.cap(1)+"="+settingRegex.cap(2);
            QMultiMap<QString, QString>::const_iterator i = mUserSettings.find(key);
            while (i != mUserSettings.end() && i.key() == key)
            {
                QString settingLine = i.key() + "=" + i.value();
                if (settingRegex.indexIn(settingLine) != -1)
                {
                    if ((settingRegex.cap(1)+"="+settingRegex.cap(2)) == keyVal)
                    {
                        *iter = settingLine;
                        break;
                    }
                }
                ++i;
            }
        }
    }

    // comments at top of file
    for (auto & iter : fileCopy)
    {
        if (iter.isNull())
            continue;

        // Below is based on readFile() code, if that changes corresponding change may be
        // required (for example duplicates may be inserted if the rules don't match)
        if (/*(*iter).isEmpty() ||*/ iter.contains(QRegExp("^\\s*#")))
        {
            stream << iter << "\n";
            continue;
        }
    }

    // Iterate in reverse order to preserve insertion order
    QString settingLine;
    QMapIterator<QString, QString> it(mUserSettings);
    it.toBack();

    while (it.hasPrevious())
    {
        it.previous();

        if (it.key() == QLatin1String("data")
            || it.key() == QLatin1String("data-local")
            || it.key() == QLatin1String("resources")
            || it.key() == QLatin1String("load-savegame"))
        {
            settingLine = it.key() + "=";

            // The following is based on boost::io::detail::quoted_manip.hpp, but calling those functions did not work as there are too may QStrings involved
            QChar delim = '\"';
            QChar escape = '&';
            QString string = it.value();

            settingLine += delim;
            for (auto iter : string)
            {
                if (iter == delim || iter == escape)
                    settingLine += escape;
                settingLine += iter;
            }
            settingLine += delim;
        }
        else
            settingLine = it.key() + "=" + it.value();

        if (settingRegex.indexIn(settingLine) != -1)
        {
            auto i = commentsMap.find(settingRegex.cap(1)+"="+settingRegex.cap(2));

            // check if previous removed content item with comments
            if (i == commentsMap.end())
                i = commentsMap.find("##"+settingRegex.cap(1)+"="+settingRegex.cap(2));

            if (i != commentsMap.end())
            {
                std::vector<QString> cLines = i->second;
                for (const auto & cLine : cLines)
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
            if (i->first.contains(QRegExp("^\\s*content\\s*=")))
            {
                std::vector<QString> cLines = i->second;
                for (const auto & cLine : cLines)
                    stream << cLine << "\n";

                // mark the content line entry for future preocessing
                stream << "##" << i->first << "\n";

                //commentsMap.erase(i);
            }
        }
    }

    // flush any end comments
    if (!comments.empty())
    {
        for (const auto & comment : comments)
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
        if (content.at(i).endsWith(QLatin1String(".omwgame"), Qt::CaseInsensitive) || content.at(i).endsWith(QLatin1String(".esm"), Qt::CaseInsensitive)) 
        {
            result = true;
            break;
        }
    }

    return result;
}

void Config::GameSettings::setContentList(const QStringList& fileNames)
{
    remove(sContentKey);
    for (const QString& fileName : fileNames)
    {
        setMultiValue(sContentKey, fileName);
    }
}

QStringList Config::GameSettings::getContentList() const
{
    // QMap returns multiple rows in LIFO order, so need to reverse
    return Config::LauncherSettings::reverse(values(sContentKey));
}

void Config::GameSettings::clear()
{
    mSettings.clear();
    mUserSettings.clear();
    mDataDirs.clear();
    mDataLocal.clear();
}

