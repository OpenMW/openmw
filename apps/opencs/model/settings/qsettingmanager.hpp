#ifndef CSMSETTINGS_QSETTINGMANAGER_HPP
#define CSMSETTINGS_QSETTINGMANAGER_HPP

#include <QSettings>
#include <QString>

#include "./setting.hpp"

namespace CSMSettings
{
    class QSettingManager
    {
        QSettings mSettings;
        static QSettingManager* sThisPointer;

    public:
        static QSettingManager* getSettingManager();
        QSettingManager(const QString& filePath);

        Setting findSetting(const QString& key) const;

        void storeSetting(const QString& key, const Setting& setting);
    };
}
#endif // CSMSETTINGS_QSETTINGMANAGER_HPP
