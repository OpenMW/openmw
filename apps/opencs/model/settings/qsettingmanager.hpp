#ifndef CSMSETTINGS_QSETTINGMANAGER_HPP
#define CSMSETTINGS_QSETTINGMANAGER_HPP

#include <QSettings>
#include <QString>

namespace CSMSettings
{
    class QSettingManager
    {
        QSettings mSettings;
        static QSettingManager* sThisPointer;

    public:
        static QSettingManager* getSettingManager();
        QSettingManager(const QString& filePath);

        //you should know what kind of data type you want to retrive from the qvariant
        QVariant findSetting(const QString& key) const;

        template <typename T>
        void storeSetting(const QString& key, const T& value)
        {
            mSettings.setValue(key, value);
        }
    };
}
#endif // CSMSETTINGS_QSETTINGMANAGER_HPP
