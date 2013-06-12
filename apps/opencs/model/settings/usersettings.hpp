#ifndef USERSETTINGS_HPP
#define USERSETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QMap>

#include <boost/filesystem/path.hpp>

#include "support.hpp"

namespace Files { typedef std::vector<boost::filesystem::path> PathContainer;
                  struct ConfigurationManager;}

class QFile;

namespace CSMSettings {

    struct UserSettings: public QObject
    {

        Q_OBJECT

    public:

        UserSettings();
        ~UserSettings();

        static const UserSettings& instance();

        void readSettings();
        void setSettingsFiles(QStringList files);

        QFile *openFile (const QString &) const;
        bool writeFile(QFile *file, QMap<QString, SettingList *> &sections) const;
        void getSettings (QTextStream &stream, SectionMap &settings) const;
        QStringList getSettingsFiles () const;
        CSMSettings::SectionMap getSettingsMap() const;
        QString getSettingValue(QString section, QString setting) const;

    private:

        static UserSettings *mUserSettingsInstance;

        CSMSettings::SectionMap mSectionMap;
        QStringList mSettingsFiles;

        UserSettings (UserSettings const &);        //not implemented
        void operator= (UserSettings const &);      //not implemented

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    };
}
#endif // USERSETTINGS_HPP
