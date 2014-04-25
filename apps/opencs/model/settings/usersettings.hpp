#ifndef USERSETTINGS_HPP
#define USERSETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QMap>

#include <boost/filesystem/path.hpp>

#include "settingmanager.hpp"

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

namespace Files { typedef std::vector<boost::filesystem::path> PathContainer;
                  struct ConfigurationManager;}

class QFile;

namespace CSMSettings {

    class UserSettings: public SettingManager
    {

        Q_OBJECT

        static UserSettings *mUserSettingsInstance;
        QString mUserFilePath;
        Files::ConfigurationManager mCfgMgr;

        QString mReadOnlyMessage;
        QString mReadWriteMessage;

    public:

        /// Singleton implementation
        static UserSettings& instance();

        UserSettings();
        ~UserSettings();

        UserSettings (UserSettings const &);        //not implemented
        void operator= (UserSettings const &);      //not implemented

        /// Writes settings to the last loaded settings file
        bool writeSettings();

        /// Retrieves the settings file at all three levels (global, local and user).
        void loadSettings (const QString &fileName);

        /// Writes settings to the user's config file path
        void saveSettings (const QMap <QString, QStringList > &settingMap);

        QString settingValue (const QString &settingKey);

    private:

        void buildSettingModelDefaults();
    };
}
#endif // USERSETTINGS_HPP
