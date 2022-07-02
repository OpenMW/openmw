#ifndef GAMESETTINGS_HPP
#define GAMESETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QMultiMap>

#include <boost/filesystem/path.hpp>

namespace Files
{
  typedef std::vector<boost::filesystem::path> PathContainer;
  struct ConfigurationManager;
}

namespace Config
{
    class GameSettings
    {
    public:
        GameSettings(Files::ConfigurationManager &cfg);

        inline QString value(const QString &key, const QString &defaultValue = QString())
        {
            return mSettings.value(key).isEmpty() ? defaultValue : mSettings.value(key);
        }


        inline void setValue(const QString &key, const QString &value)
        {
            mSettings.remove(key);
            mSettings.insert(key, value);
            mUserSettings.remove(key);
            mUserSettings.insert(key, value);
        }

        inline void setMultiValue(const QString &key, const QString &value)
        {
            QStringList values = mSettings.values(key);
            if (!values.contains(value))
                mSettings.insert(key, value);

            values = mUserSettings.values(key);
            if (!values.contains(value))
                mUserSettings.insert(key, value);
        }

        inline void remove(const QString &key)
        {
            mSettings.remove(key);
            mUserSettings.remove(key);
        }

        QStringList getDataDirs() const;
        std::string getGlobalDataDir() const;

        inline void removeDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.removeAll(dir); }
        inline void addDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.append(dir); }
        inline QString getDataLocal() const {return mDataLocal; }

        bool hasMaster();

        QStringList values(const QString &key, const QStringList &defaultValues = QStringList()) const;

        bool readFile(QTextStream &stream, bool ignoreContent = false);
        bool readFile(QTextStream &stream, QMultiMap<QString, QString> &settings, bool ignoreContent = false);
        bool readUserFile(QTextStream &stream, bool ignoreContent = false);

        bool writeFile(QTextStream &stream);
        bool writeFileWithComments(QFile &file);

        QStringList getArchiveList() const;
        void setContentList(const QStringList& dirNames, const QStringList& archiveNames, const QStringList& fileNames);
        QStringList getContentList() const;

        void clear();

    private:
        Files::ConfigurationManager &mCfgMgr;

        void validatePaths();
        QMultiMap<QString, QString> mSettings;
        QMultiMap<QString, QString> mUserSettings;

        QStringList mDataDirs;
        QString mDataLocal;

        static const char sArchiveKey[];
        static const char sContentKey[];
        static const char sDirectoryKey[];

        static bool isOrderedLine(const QString& line) ;
    };
}
#endif // GAMESETTINGS_HPP
