#include "./qsettingmanager.hpp"

#include <cassert>

CSMSettings::QSettingManager* CSMSettings::QSettingManager::sThisPointer = NULL;

CSMSettings::QSettingManager::QSettingManager(const QString& filePath) :
mSettings(filePath)    //QSETTINGS WILL BE UNABLE TO DEAL WITH NON-QSETTINGS SYNTAX!!!
{
    assert(!sThisPointer);
    sThisPointer = this;
}

QVariant CSMSettings::QSettingManager::findSetting (const QString& key) const
{
    return mSettings.value(key);
}

CSMSettings::QSettingManager* CSMSettings::QSettingManager::getSettingManager()
{
    return sThisPointer;
}
