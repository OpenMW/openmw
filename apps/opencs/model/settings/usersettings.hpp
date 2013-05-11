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

    class UserSettings
    {
    public:
        UserSettings(Files::ConfigurationManager &cfg);
        ~UserSettings();

        QFile *openFile (const QString &);
        bool writeFile(QFile *file, QMap<QString, SettingList *> &sections);
        void getSettings (QTextStream &stream, SectionMap &settings);

    private:
        Files::ConfigurationManager &mCfgMgr;

    };
}
#endif // USERSETTINGS_HPP
