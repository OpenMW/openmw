#ifndef USERSETTINGS_HPP
#define USERSETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QMap>

#include <boost/filesystem/path.hpp>

#include "support.hpp"

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

namespace Files { typedef std::vector<boost::filesystem::path> PathContainer;
                  struct ConfigurationManager;}

class QFile;

namespace CSMSettings {

    struct UserSettings: public QObject
    {

        Q_OBJECT

        SectionMap mSectionSettings;
        UserSettings *mUserSettingsInstance;
        QStringList mPaths;
        Files::ConfigurationManager mCfgMgr;

        QString mReadOnlyMessage;
        QString mReadWriteMessage;

    public:

        UserSettings();
        ~UserSettings();

        static const UserSettings& instance();

        void readSettings();
        void setSettingsFiles(QStringList files);

<<<<<<< HEAD
        bool writeFile(QMap<QString, SettingList *> &sections);
        const SectionMap &getSettings ();
        void updateSettings (const QString &sectionName, const QString &settingName = "");
        void loadSettings (const QString &fileName);

    private:

        UserSettings();
        ~UserSettings();
=======
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
>>>>>>> df1f1bd5c81d94a1ea2693000ec5dc589b069826

        UserSettings (UserSettings const &);        //not implemented
        void operator= (UserSettings const &);      //not implemented

        QTextStream *openFileStream (const QString &filePath, bool isReadOnly = false);
        void loadFromFile (const QString &filePath = "");

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    };
}
#endif // USERSETTINGS_HPP
