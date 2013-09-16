#ifndef USERSETTINGS_HPP
#define USERSETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QMap>
#include <QSortFilterProxyModel>

#include <boost/filesystem/path.hpp>

#include "support.hpp"
#include "settingmodel.hpp"

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

        SettingModel *mSettingModel;

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

        /// \todo Multi-valued settings are not fully implemented.  Setting values
        /// \todo loaded in later files will always overwrite previously loaded values.
        void loadSettings (const QString &fileName);

        SettingModel *settingModel()  { return mSettingModel; }

    private:


        /// Opens a QTextStream from the provided path as read-only or read-write.
        QTextStream *openFileStream (const QString &filePath, bool isReadOnly = false) const;

        void displayFileErrorMessage(const QString &message, bool isReadOnly);

        bool loadSettingsFromFile (const QString &filePath);

        void buildSettingModelDefaults();

        void destroyStream (QTextStream *stream) const;

        QSortFilterProxyModel *createProxyFilter (int column, QAbstractItemModel *model = 0);

    signals:

        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    };
}
#endif // USERSETTINGS_HPP
