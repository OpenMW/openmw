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
        SectionMap mEditorSettingDefaults;

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
        bool writeSettings(QMap<QString, SettingList *> &sections);

        /// Called from editor to trigger signal to update the specified setting.
        /// If no setting name is specified, all settings found in the specified section are updated.
        void updateSettings (const QString &sectionName, const QString &settingName = "");

        /// Retrieves the settings file at all three levels (global, local and user).

        /// \todo Multi-valued settings are not fully implemented.  Setting values
        /// \todo loaded in later files will always overwrite previously loaded values.
        void loadSettings (const QString &fileName);

        /// Returns the entire map of settings across all sections
        const SectionMap &getSectionMap () const;

        const SettingMap *getSettings (const QString &sectionName) const;

        /// Retrieves the value as a QString of the specified setting in the specified section
        QString getSetting(const QString &section, const QString &setting) const;

    private:


        /// Opens a QTextStream from the provided path as read-only or read-write.
        QTextStream *openFileStream (const QString &filePath, bool isReadOnly = false) const;

        ///  Parses a setting file specified in filePath from the provided text stream.
        bool loadFromFile (const QString &filePath = "");

        /// merge the passed map into mSectionSettings
        void mergeMap (const SectionMap &);

        void displayFileErrorMessage(const QString &message, bool isReadOnly);

        void buildEditorSettingDefaults();

        SettingMap *getValidSettings (const QString &sectionName) const;

    signals:

        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    };
}
#endif // USERSETTINGS_HPP
