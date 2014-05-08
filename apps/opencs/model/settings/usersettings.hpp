#ifndef USERSETTINGS_HPP
#define USERSETTINGS_HPP

#include <QList>
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
class QSettings;

namespace CSMSettings {

    class Setting;
    typedef QMap <QString, QList <Setting *> > SettingPageMap;

    class UserSettings: public QObject
    {

        Q_OBJECT

        static UserSettings *mUserSettingsInstance;
        Files::ConfigurationManager mCfgMgr;

        QSettings *mSettingDefinitions;
        QList <Setting *> mSettings;

    public:

        /// Singleton implementation
        static UserSettings& instance();

        UserSettings();
        ~UserSettings();

        UserSettings (UserSettings const &);        //not implemented
        void operator= (UserSettings const &);      //not implemented

        /// Retrieves the settings file at all three levels (global, local and user).
        void loadSettings (const QString &fileName);

        /// Updates QSettings and syncs with the ini file
        void setDefinitions (const QString &key, const QStringList &defs);

        QString settingValue (const QString &settingKey);

        ///retrieve a setting object from a given page and setting name
        Setting *findSetting
            (const QString &pageName, const QString &settingName = QString());

        ///remove a setting from the list
        void removeSetting
                        (const QString &pageName, const QString &settingName);

        ///Retreive a map of the settings, keyed by page name
        SettingPageMap settingPageMap() const;

        ///Returns a string list of defined vlaues for the specified setting
        ///in "page/name" format.
        QStringList definitions (const QString &viewKey) const;

        ///Test to indicate whether or not a setting has any definitions
        bool hasSettingDefinitions (const QString &viewKey) const;

        ///Save any unsaved changes in the QSettings object
        void saveDefinitions() const;

    private:

        void buildSettingModelDefaults();

        ///add a new setting to the model and return it
        Setting *createSetting (CSMSettings::SettingType typ,
                            const QString &page, const QString &name);

    signals:

        void userSettingUpdated (const QString &, const QStringList &);

    public slots:

        void updateUserSetting (const QString &, const QStringList &);
    };
}
#endif // USERSETTINGS_HPP
