#include <QFile>
#include <QTextCodec>
#include <QMessageBox>
#include <QDebug>
#include <QList>
#include <QSettings>

#include "setting.hpp"
#include "settingmanager.hpp"

CSMSettings::SettingManager::SettingManager(QObject *parent) :
    QObject(parent)
{}

CSMSettings::Setting *CSMSettings::SettingManager::createSetting
        (CSMSettings::SettingType typ, const QString &page, const QString &name)
{
    //get list of all settings for the current setting name
    if (findSetting (page, name))
    {
        qWarning() << "Duplicate declaration encountered: "
                   << (name + '/' + page);
        return 0;
    }

    Setting *setting = new Setting (typ, name, page);


    //add declaration to the model
    mSettings.append (setting);

    return setting;
}

void CSMSettings::SettingManager::addDefinitions (const QSettings *settings)
{
    foreach (const QString &key, settings->allKeys())
    {
        QStringList names = key.split('/');

        Setting *setting = findSetting (names.at(0), names.at(1));

        if (!setting)
        {
            qWarning() << "Found definitions for undeclared setting "
                       << names.at(0) << "." << names.at(1);
            continue;
        }

        QStringList values = settings->value (key).toStringList();

        if (values.isEmpty())
            values.append (setting->defaultValues());

        setting->setDefinedValues (values);
    }
}

CSMSettings::Setting *CSMSettings::SettingManager::findSetting
                        (const QString &pageName, const QString &settingName)
{
    foreach (Setting *setting, mSettings)
    {
        if (setting->name() == settingName)
        {
            if (setting->page() == pageName)
                return setting;
        }
    }
    return 0;
}

CSMSettings::SettingPageMap CSMSettings::SettingManager::settingPageMap() const
{
    SettingPageMap pageMap;

    foreach (Setting *setting, mSettings)
        pageMap[setting->page()].append (setting);

    return pageMap;
}

void CSMSettings::SettingManager::updateUserSetting(const QString &settingKey,
                                                    const QStringList &list)
{
    QStringList names = settingKey.split('/');

    Setting *setting = findSetting (names.at(0), names.at(1));

    setting->setDefinedValues (list);

    emit userSettingUpdated (settingKey, list);
}
