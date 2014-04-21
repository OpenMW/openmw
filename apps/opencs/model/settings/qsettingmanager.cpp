#include "./qsettingmanager.hpp"

#include <QMetaType>

#include <cassert>

CSMSettings::QSettingManager* CSMSettings::QSettingManager::sThisPointer = NULL;

CSMSettings::QSettingManager::QSettingManager(const QString& filePath) :
mSettings(filePath)    //QSETTINGS WILL BE UNABLE TO DEAL WITH NON-QSETTINGS SYNTAX!!!
{
    assert(!sThisPointer);
    sThisPointer = this;
    qRegisterMetaType<CSMSettings::Setting>("SettingStruct");
    qRegisterMetaTypeStreamOperators<CSMSettings::Setting>("SettingStruct");
}

CSMSettings::Setting CSMSettings::QSettingManager::findSetting (const QString& key) const
{
    return mSettings.value(key).value<CSMSettings::Setting>();
}

CSMSettings::QSettingManager* CSMSettings::QSettingManager::getSettingManager()
{
    return sThisPointer;
}

void CSMSettings::QSettingManager::storeSetting (const QString& key, const Setting& setting)
{
    mSettings.setValue(key, QVariant::fromValue(setting));
}
