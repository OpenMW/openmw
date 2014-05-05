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
{
    mReadWriteMessage = QObject::tr("<br><b>Could not open or create file for \
                        writing</b><br><br> Please make sure you have the right\
                         permissions and try again.<br>");

    mReadOnlyMessage = QObject::tr("<br><b>Could not open file for \
                        reading</b><br><br> Please make sure you have the \
                        right permissions and try again.<br>");

}

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

void CSMSettings::SettingManager::displayFileErrorMessage(const QString &message,
                                                        bool isReadOnly) const
{
        // File cannot be opened or created
        QMessageBox msgBox;
        msgBox.setWindowTitle(QObject::tr("OpenCS configuration file I/O error"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);

        if (!isReadOnly)
            msgBox.setText (mReadWriteMessage + message);
        else
            msgBox.setText (message);

        msgBox.exec();
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

        qDebug() << "added definitons " << values;
    }
}

QList <CSMSettings::Setting *> CSMSettings::SettingManager::findSettings
                                                    (const QStringList &list)
{
    QList <CSMSettings::Setting *> settings;

    foreach (const QString &value, list)
    {
        QStringList names = value.split(".", QString::SkipEmptyParts);

        if (names.size() != 2)
            continue;

        Setting *setting = findSetting (names.at(0), names.at(1));

        if (!setting)
            continue;

        settings.append (setting);
    }

    return settings;
}


CSMSettings::Setting *CSMSettings::SettingManager::findSetting
                        (const QString &pageName, const QString &settingName)
{
    foreach (Setting *setting, mSettings)
    {
        if (settingName.isEmpty() || (setting->name() == settingName))
        {
            if (setting->page() == pageName)
                return setting;
        }
    }
    return 0;
}
/*
QList <CSMSettings::Setting *> CSMSettings::SettingManager::findSettings
                                                    (const QString &pageName)
{
    QList <CSMSettings::Setting *> settings;

    foreach (Setting *setting, mSettings)
    {
        if (setting->page() == pageName)
            settings.append (setting);
    }
    return settings;
}
*/
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
