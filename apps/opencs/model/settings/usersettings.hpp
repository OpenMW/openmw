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

        static UserSettings &instance()
        {
            static UserSettings instance;

            return instance;
        }

        bool writeFile(QMap<QString, SettingList *> &sections);
        const SectionMap &getSettings ();
        void updateSettings (const QString &sectionName, const QString &settingName = "");
        void loadSettings (const QString &fileName);

    private:

        UserSettings();
        ~UserSettings();

        UserSettings (UserSettings const &);        //not implemented
        void operator= (UserSettings const &);      //not implemented

        QTextStream *openFileStream (const QString &filePath, bool isReadOnly = false);
        void loadFromFile (const QString &filePath = "");

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    };
}
#endif // USERSETTINGS_HPP
