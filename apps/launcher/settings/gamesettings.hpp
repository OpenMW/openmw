#ifndef GAMESETTINGS_HPP
#define GAMESETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QMap>

#include <boost/filesystem/path.hpp>

namespace Files { typedef std::vector<boost::filesystem::path> PathContainer;
                  struct ConfigurationManager;}

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
    }

    inline void setMultiValue(const QString &key, const QString &value)
    {
        QStringList values = mSettings.values(key);
        if (!values.contains(value))
            mSettings.insertMulti(key, value);
    }

    inline void remove(const QString &key)
    {
        mSettings.remove(key);
    }

    inline QStringList getDataDirs() { return mDataDirs; }
    inline void addDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.append(dir); }
    inline QString getDataLocal() {return mDataLocal; }

    QStringList values(const QString &key, const QStringList &defaultValues = QStringList());
    bool readFile(QTextStream &stream);
    bool writeFile(QTextStream &stream);

private:
    Files::ConfigurationManager &mCfgMgr;

    void validatePaths();
    QMap<QString, QString> mSettings;

    QStringList mDataDirs;
    QString mDataLocal;
};

#endif // GAMESETTINGS_HPP
