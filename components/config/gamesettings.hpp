#ifndef GAMESETTINGS_HPP
#define GAMESETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QMap>

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
        ~GameSettings();

        inline QString value(const QString &key, const QString &defaultValue = QString())
        {
            return mSettings.value(key).isEmpty() ? defaultValue : mSettings.value(key);
        }


        inline void setValue(const QString &key, const QString &value)
        {
            mSettings.insert(key, value);
            mUserSettings.insert(key, value);
        }

        inline void setMultiValue(const QString &key, const QString &value)
        {
            QStringList values = mSettings.values(key);
            if (!values.contains(value))
                mSettings.insertMulti(key, value);

            values = mUserSettings.values(key);
            if (!values.contains(value))
                mUserSettings.insertMulti(key, value);
        }

        inline void remove(const QString &key)
        {
            mSettings.remove(key);
            mUserSettings.remove(key);
        }

        inline QStringList getDataDirs() { return mDataDirs; }

        inline void removeDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.removeAll(dir); }
        inline void addDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.append(dir); }
        inline QString getDataLocal() {return mDataLocal; }

        bool hasMaster();

        QStringList values(const QString &key, const QStringList &defaultValues = QStringList()) const;

        bool readFile(QTextStream &stream);
        bool readFile(QTextStream &stream, QMap<QString, QString> &settings);
        bool readUserFile(QTextStream &stream);

        bool writeFile(QTextStream &stream);

        void setContentList(const QStringList& fileNames);
        QStringList getContentList() const;

    private:
        Files::ConfigurationManager &mCfgMgr;

        void validatePaths();
        QMap<QString, QString> mSettings;
        QMap<QString, QString> mUserSettings;

        QStringList mDataDirs;
        QString mDataLocal;

        static const char sContentKey[];
    };
}
#endif // GAMESETTINGS_HPP
