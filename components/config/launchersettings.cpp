#include "launchersettings.hpp"

#include <QDebug>
#include <QDir>
#include <QMultiMap>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <components/debug/debuglog.hpp>
#include <components/files/qtconversion.hpp>

#include "gamesettings.hpp"

namespace Config
{
    namespace
    {
        constexpr char sSettingsSection[] = "Settings";
        constexpr char sGeneralSection[] = "General";
        constexpr char sProfilesSection[] = "Profiles";
        constexpr char sLanguageKey[] = "language";
        constexpr char sCurrentProfileKey[] = "currentprofile";
        constexpr char sDataKey[] = "data";
        constexpr char sArchiveKey[] = "fallback-archive";
        constexpr char sContentKey[] = "content";
        constexpr char sFirstRunKey[] = "firstrun";
        constexpr char sMainWindowWidthKey[] = "MainWindow/width";
        constexpr char sMainWindowHeightKey[] = "MainWindow/height";
        constexpr char sMainWindowPosXKey[] = "MainWindow/posx";
        constexpr char sMainWindowPosYKey[] = "MainWindow/posy";

        QString makeNewContentListName()
        {
            // basically, use date and time as the name  e.g. YYYY-MM-DDThh:mm:ss
            const std::time_t rawtime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            tm timeinfo{};
#ifdef _WIN32
            (void)localtime_s(&timeinfo, &rawtime);
#else
            (void)localtime_r(&rawtime, &timeinfo);
#endif
            constexpr int base = 10;
            QChar zeroPad('0');
            return QString("%1-%2-%3T%4:%5:%6")
                .arg(timeinfo.tm_year + 1900, 4)
                .arg(timeinfo.tm_mon + 1, 2, base, zeroPad)
                .arg(timeinfo.tm_mday, 2, base, zeroPad)
                .arg(timeinfo.tm_hour, 2, base, zeroPad)
                .arg(timeinfo.tm_min, 2, base, zeroPad)
                .arg(timeinfo.tm_sec, 2, base, zeroPad);
        }

        bool parseBool(const QString& value, bool& out)
        {
            if (value == "false")
            {
                out = false;
                return true;
            }
            if (value == "true")
            {
                out = true;
                return true;
            }

            return false;
        }

        bool parseInt(const QString& value, int& out)
        {
            bool ok = false;
            const int converted = value.toInt(&ok);
            if (ok)
                out = converted;
            return ok;
        }

        bool parseProfilePart(
            const QString& key, const QString& value, std::map<QString, LauncherSettings::Profile>& profiles)
        {
            const int separator = key.lastIndexOf('/');
            if (separator == -1)
                return false;

            const QString profileName = key.mid(0, separator);

            if (key.endsWith(sArchiveKey))
            {
                profiles[profileName].mArchives.append(value);
                return true;
            }
            if (key.endsWith(sDataKey))
            {
                profiles[profileName].mData.append(value);
                return true;
            }
            if (key.endsWith(sContentKey))
            {
                profiles[profileName].mContent.append(value);
                return true;
            }

            return false;
        }

        bool parseSettingsSection(const QString& key, const QString& value, LauncherSettings::Settings& settings)
        {
            if (key == sLanguageKey)
            {
                settings.mLanguage = value;
                return true;
            }

            return false;
        }

        bool parseProfilesSection(const QString& key, const QString& value, LauncherSettings::Profiles& profiles)
        {
            if (key == sCurrentProfileKey)
            {
                profiles.mCurrentProfile = value;
                return true;
            }

            return parseProfilePart(key, value, profiles.mValues);
        }

        bool parseGeneralSection(const QString& key, const QString& value, LauncherSettings::General& general)
        {
            if (key == sFirstRunKey)
                return parseBool(value, general.mFirstRun);
            if (key == sMainWindowWidthKey)
                return parseInt(value, general.mMainWindow.mWidth);
            if (key == sMainWindowHeightKey)
                return parseInt(value, general.mMainWindow.mHeight);
            if (key == sMainWindowPosXKey)
                return parseInt(value, general.mMainWindow.mPosX);
            if (key == sMainWindowPosYKey)
                return parseInt(value, general.mMainWindow.mPosY);

            return false;
        }

        template <std::size_t size>
        void writeSectionHeader(const char (&name)[size], QTextStream& stream)
        {
            stream << "\n[" << name << "]\n";
        }

        template <std::size_t size>
        void writeKeyValue(const char (&key)[size], const QString& value, QTextStream& stream)
        {
            stream << key << '=' << value << '\n';
        }

        template <std::size_t size>
        void writeKeyValue(const char (&key)[size], bool value, QTextStream& stream)
        {
            stream << key << '=' << (value ? "true" : "false") << '\n';
        }

        template <std::size_t size>
        void writeKeyValue(const char (&key)[size], int value, QTextStream& stream)
        {
            stream << key << '=' << value << '\n';
        }

        template <std::size_t size>
        void writeKeyValues(
            const QString& prefix, const char (&suffix)[size], const QStringList& values, QTextStream& stream)
        {
            for (const auto& v : values)
                stream << prefix << '/' << suffix << '=' << v << '\n';
        }

        void writeSettings(const LauncherSettings::Settings& value, QTextStream& stream)
        {
            writeSectionHeader(sSettingsSection, stream);
            writeKeyValue(sLanguageKey, value.mLanguage, stream);
        }

        void writeProfiles(const LauncherSettings::Profiles& value, QTextStream& stream)
        {
            writeSectionHeader(sProfilesSection, stream);
            writeKeyValue(sCurrentProfileKey, value.mCurrentProfile, stream);
            for (auto it = value.mValues.rbegin(); it != value.mValues.rend(); ++it)
            {
                writeKeyValues(it->first, sArchiveKey, it->second.mArchives, stream);
                writeKeyValues(it->first, sDataKey, it->second.mData, stream);
                writeKeyValues(it->first, sContentKey, it->second.mContent, stream);
            }
        }

        void writeGeneral(const LauncherSettings::General& value, QTextStream& stream)
        {
            writeSectionHeader(sGeneralSection, stream);
            writeKeyValue(sFirstRunKey, value.mFirstRun, stream);
            writeKeyValue(sMainWindowWidthKey, value.mMainWindow.mWidth, stream);
            writeKeyValue(sMainWindowPosYKey, value.mMainWindow.mPosY, stream);
            writeKeyValue(sMainWindowPosXKey, value.mMainWindow.mPosX, stream);
            writeKeyValue(sMainWindowHeightKey, value.mMainWindow.mHeight, stream);
        }
    }
}

void Config::LauncherSettings::writeFile(QTextStream& stream) const
{
    writeSettings(mSettings, stream);
    writeProfiles(mProfiles, stream);
    writeGeneral(mGeneral, stream);
}

QStringList Config::LauncherSettings::getContentLists()
{
    QStringList result;
    result.reserve(mProfiles.mValues.size());
    for (const auto& [k, v] : mProfiles.mValues)
        result.push_back(k);
    return result;
}

void Config::LauncherSettings::setContentList(const GameSettings& gameSettings)
{
    // obtain content list from game settings (if present)
    QList<SettingValue> dirs(gameSettings.getDataDirs());
    dirs.erase(std::remove_if(
                   dirs.begin(), dirs.end(), [&](const SettingValue& dir) { return !gameSettings.isUserSetting(dir); }),
        dirs.end());
    // archives and content files aren't preprocessed, so we don't need to track their original form
    const QList<SettingValue> archivesOriginal(gameSettings.getArchiveList());
    QStringList archives;
    for (const auto& archive : archivesOriginal)
    {
        if (gameSettings.isUserSetting(archive))
            archives.push_back(archive.value);
    }
    const QList<SettingValue> filesOriginal(gameSettings.getContentList());
    QStringList files;
    for (const auto& file : filesOriginal)
    {
        if (gameSettings.isUserSetting(file))
            files.push_back(file.value);
    }

    // if openmw.cfg has no content, exit so we don't create an empty content list.
    if (dirs.isEmpty() || files.isEmpty())
    {
        return;
    }

    // local data directory and resources/vfs are not part of any profile
    const auto resourcesVfs = gameSettings.getResourcesVfs();
    const auto dataLocal = gameSettings.getDataLocal();
    dirs.erase(
        std::remove_if(dirs.begin(), dirs.end(), [&](const SettingValue& dir) { return dir.value == resourcesVfs; }),
        dirs.end());
    dirs.erase(
        std::remove_if(dirs.begin(), dirs.end(), [&](const SettingValue& dir) { return dir.value == dataLocal; }),
        dirs.end());

    // if any existing profile in launcher matches the content list, make that profile the default
    for (const QString& listName : getContentLists())
    {
        const auto& listDirs = getDataDirectoryList(listName);
#ifdef Q_OS_WINDOWS
        constexpr auto caseSensitivity = Qt::CaseInsensitive;
#else
        constexpr auto caseSensitivity = Qt::CaseSensitive;
#endif
        constexpr auto compareDataDirectories = [caseSensitivity](const SettingValue& dir, const QString& listDir) {
            return dir.originalRepresentation == listDir
                || QDir::cleanPath(dir.originalRepresentation).compare(QDir::cleanPath(listDir), caseSensitivity) == 0;
        };
        if (!std::ranges::equal(dirs, listDirs, compareDataDirectories))
            continue;
        constexpr auto compareFiles
            = [](const QString& a, const QString& b) { return a.compare(b, Qt::CaseInsensitive) == 0; };
        if (!std::ranges::equal(files, getContentListFiles(listName), compareFiles))
            continue;
        if (!std::ranges::equal(archives, getArchiveList(listName), compareFiles))
            continue;
        setCurrentContentListName(listName);
        return;
    }

    // otherwise, add content list
    QString newContentListName(makeNewContentListName());
    setCurrentContentListName(newContentListName);
    QStringList newListDirs;
    for (const auto& dir : dirs)
        newListDirs.push_back(dir.originalRepresentation);
    setContentList(newContentListName, newListDirs, archives, files);
}

void Config::LauncherSettings::setContentList(const QString& contentListName, const QStringList& dirNames,
    const QStringList& archiveNames, const QStringList& fileNames)
{
    Profile& profile = mProfiles.mValues[contentListName];
    profile.mData = dirNames;
    profile.mArchives = archiveNames;
    profile.mContent = fileNames;
}

QStringList Config::LauncherSettings::getDataDirectoryList(const QString& contentListName) const
{
    const Profile* profile = findProfile(contentListName);
    if (profile == nullptr)
        return {};
    return profile->mData;
}

QStringList Config::LauncherSettings::getArchiveList(const QString& contentListName) const
{
    const Profile* profile = findProfile(contentListName);
    if (profile == nullptr)
        return {};
    return profile->mArchives;
}

QStringList Config::LauncherSettings::getContentListFiles(const QString& contentListName) const
{
    const Profile* profile = findProfile(contentListName);
    if (profile == nullptr)
        return {};
    return profile->mContent;
}

bool Config::LauncherSettings::setValue(const QString& sectionPrefix, const QString& key, const QString& value)
{
    if (sectionPrefix == sSettingsSection)
        return parseSettingsSection(key, value, mSettings);
    if (sectionPrefix == sProfilesSection)
        return parseProfilesSection(key, value, mProfiles);
    if (sectionPrefix == sGeneralSection)
        return parseGeneralSection(key, value, mGeneral);

    return false;
}

void Config::LauncherSettings::readFile(QTextStream& stream)
{
    const QRegularExpression sectionRe("^\\[([^]]+)\\]$");
    const QRegularExpression keyRe("^([^=]+)\\s*=\\s*(.+)$");

    QString section;

    while (!stream.atEnd())
    {
        const QString line = stream.readLine();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        const QRegularExpressionMatch sectionMatch = sectionRe.match(line);
        if (sectionMatch.hasMatch())
        {
            section = sectionMatch.captured(1);
            continue;
        }

        if (section.isEmpty())
            continue;

        const QRegularExpressionMatch keyMatch = keyRe.match(line);
        if (!keyMatch.hasMatch())
            continue;

        const QString key = keyMatch.captured(1).trimmed();
        const QString value = keyMatch.captured(2).trimmed();

        if (!setValue(section, key, value))
            Log(Debug::Warning) << "Unsupported setting in the launcher config file: section: "
                                << section.toUtf8().constData() << " key: " << key.toUtf8().constData()
                                << " value: " << value.toUtf8().constData();
    }
}

const Config::LauncherSettings::Profile* Config::LauncherSettings::findProfile(const QString& name) const
{
    const auto it = mProfiles.mValues.find(name);
    if (it == mProfiles.mValues.end())
        return nullptr;
    return &it->second;
}

void Config::LauncherSettings::clear()
{
    mSettings = Settings{};
    mGeneral = General{};
    mProfiles = Profiles{};
}
