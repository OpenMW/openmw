#ifndef GAMESETTINGS_HPP
#define GAMESETTINGS_HPP

#include <QFile>
#include <QMultiMap>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <filesystem>

namespace Files
{
    typedef std::vector<std::filesystem::path> PathContainer;
    struct ConfigurationManager;
}

namespace Config
{
    struct SettingValue
    {
        QString value = "";
        // value as found in openmw.cfg, e.g. relative path with ?slug?
        QString originalRepresentation = value;
        // path of openmw.cfg, e.g. to resolve relative paths
        QString context = "";

        friend std::strong_ordering operator<=>(const SettingValue&, const SettingValue&) = default;
    };

    class GameSettings
    {
    public:
        explicit GameSettings(const Files::ConfigurationManager& cfg);

        inline SettingValue value(const QString& key, const SettingValue& defaultValue = {})
        {
            return mSettings.contains(key) ? mSettings.value(key) : defaultValue;
        }

        inline void setValue(const QString& key, const SettingValue& value)
        {
            remove(key);
            mSettings.insert(key, value);
            if (isUserSetting(value))
                mUserSettings.insert(key, value);
        }

        inline void setMultiValue(const QString& key, const SettingValue& value)
        {
            QList<SettingValue> values = mSettings.values(key);
            if (!values.contains(value))
                mSettings.insert(key, value);

            if (isUserSetting(value))
            {
                values = mUserSettings.values(key);
                if (!values.contains(value))
                    mUserSettings.insert(key, value);
            }
        }

        inline void remove(const QString& key)
        {
            // simplify to removeIf when Qt5 goes
            for (auto itr = mSettings.lowerBound(key); itr != mSettings.upperBound(key);)
            {
                if (isUserSetting(*itr))
                    itr = mSettings.erase(itr);
                else
                    ++itr;
            }
            mUserSettings.remove(key);
        }

        QList<SettingValue> getDataDirs() const;

        QString getResourcesVfs() const;

        inline void removeDataDir(const QString& existingDir)
        {
            if (!existingDir.isEmpty())
            {
                // non-user settings can't be removed as we can't edit the openmw.cfg they're in
                mDataDirs.erase(
                    std::remove_if(mDataDirs.begin(), mDataDirs.end(),
                        [&](const SettingValue& dir) { return isUserSetting(dir) && dir.value == existingDir; }),
                    mDataDirs.end());
            }
        }

        inline void addDataDir(const SettingValue& dir)
        {
            if (!dir.value.isEmpty())
                mDataDirs.append(dir);
        }

        inline QString getDataLocal() const { return mDataLocal; }

        bool hasMaster();

        QList<SettingValue> values(const QString& key, const QList<SettingValue>& defaultValues = {}) const;
        bool containsValue(const QString& key, const QString& value) const;

        bool readFile(QTextStream& stream, const QString& context, bool ignoreContent = false);
        bool readFile(QTextStream& stream, QMultiMap<QString, SettingValue>& settings, const QString& context,
            bool ignoreContent = false);
        bool readUserFile(QTextStream& stream, const QString& context, bool ignoreContent = false);

        bool writeFile(QTextStream& stream);
        bool writeFileWithComments(QFile& file);

        QList<SettingValue> getArchiveList() const;
        void setContentList(
            const QList<SettingValue>& dirNames, const QList<SettingValue>& archiveNames, const QStringList& fileNames);
        QList<SettingValue> getContentList() const;

        const QString& getUserContext() const { return mContexts.back(); }
        bool isUserSetting(const SettingValue& settingValue) const;

        void clear();

    private:
        const Files::ConfigurationManager& mCfgMgr;

        void validatePaths();
        QMultiMap<QString, SettingValue> mSettings;
        QMultiMap<QString, SettingValue> mUserSettings;

        QStringList mContexts;

        QList<SettingValue> mDataDirs;
        QString mDataLocal;

        static const char sArchiveKey[];
        static const char sContentKey[];
        static const char sDirectoryKey[];

        static bool isOrderedLine(const QString& line);
    };

    QDataStream& operator<<(QDataStream& out, const SettingValue& settingValue);
    QDataStream& operator>>(QDataStream& in, SettingValue& settingValue);
}

Q_DECLARE_METATYPE(Config::SettingValue)

#endif // GAMESETTINGS_HPP
