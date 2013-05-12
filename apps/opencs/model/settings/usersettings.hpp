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

    struct UserSettings
    {
    public:

        static UserSettings &instance()
        {
            static UserSettings instance;

            return instance;
        }

        QFile *openFile (const QString &);
        bool writeFile(QFile *file, QMap<QString, SettingList *> &sections);
        void getSettings (QTextStream &stream, SectionMap &settings);

    private:

        UserSettings *mUserSettingsInstance;
        UserSettings();
        ~UserSettings();

        UserSettings (UserSettings const &);        //not implemented
        void operator= (UserSettings const &);      //not implemented

    };
}
#endif // USERSETTINGS_HPP
