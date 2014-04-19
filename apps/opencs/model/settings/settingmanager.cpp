#include <QFile>
#include <QTextCodec>
#include <QMessageBox>
#include <QDebug>
#include <QList>

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

void CSMSettings::SettingManager::dumpModel()
{
    foreach (Setting *setting, mSettings)
    {
        if (setting->proxyLists().isEmpty())
            continue;
    }
}

CSMSettings::Setting *CSMSettings::SettingManager::createSetting
        (CSMSettings::SettingType typ, const QString &page, const QString &name,
         const QStringList &values)
{
    //get list of all settings for the current setting name
    if (findSetting (page, name))
    {
        qWarning() << "Duplicate declaration encountered: "
                   << (name + '.' + page);
        return 0;
    }

    Setting *setting = new Setting (typ, name, page, values);

    //add declaration to the model
    mSettings.append (setting);

    return setting;
}

CSMSettings::DefinitionPageMap
                CSMSettings::SettingManager::readFilestream (QTextStream *stream)
{
    //regEx's for page names and keys / values
    QRegExp pageRegEx ("^\\[([^]]+)\\]");
    QRegExp keyRegEx ("^([^=]+)\\s*=\\s*(.+)$");

    QString currPage = "Unassigned";

    DefinitionPageMap pageMap;

    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return pageMap;
    }

    if (stream->atEnd())
        return pageMap;

    DefinitionMap *settingMap = new DefinitionMap();
    pageMap[currPage] = settingMap;

    while (!stream->atEnd())
    {
        QString line = stream->readLine().simplified();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        //page name found
        if (pageRegEx.exactMatch(line))
        {
            currPage = pageRegEx.cap(1).simplified().trimmed();
            settingMap = new DefinitionMap();
            pageMap[currPage] = settingMap;
            continue;
        }

        //setting definition found
        if ( (keyRegEx.indexIn(line) != -1))
        {
            QString settingName = keyRegEx.cap(1).simplified();
            QString settingValue = keyRegEx.cap(2).simplified();

            if (!settingMap->contains (settingName))
                settingMap->insert (settingName, new QStringList());

            settingMap->value(settingName)->append(settingValue);
         }
    }

    //return empty map if no settings were ever added to
    if (pageMap.size() == 1)
    {
        QString pageKey = pageMap.keys().at(0);
        if (pageMap[pageKey]->size() == 0)
            pageMap.clear();
    }

    return pageMap;
}

bool CSMSettings::SettingManager::writeFilestream(QTextStream *stream,
                         const QMap <QString, QStringList > &settingListMap)
{
    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return false;
    }
    //disabled after rolling selector class into view.  Need to
    //iterate views to get setting definitions before writing to file

    QStringList sectionKeys;

    foreach (const QString &key, settingListMap.keys())
    {
        QStringList names = key.split('.');
        QString section = names.at(0);

        if (!sectionKeys.contains(section))
            if (!settingListMap.value(key).isEmpty())
                sectionKeys.append (section);
    }

    foreach (const QString &section, sectionKeys)
    {
        *stream << '[' << section << "]\n";
        foreach (const QString &key, settingListMap.keys())
        {
            QStringList names = key.split('.');

            if (names.at(0) != section)
                continue;

            QStringList list = settingListMap.value(key);

            if (list.isEmpty())
                continue;

            QString name = names.at(1);

            foreach (const QString value, list)
            {
                if (value.isEmpty())
                    continue;

                *stream << name << " = " << value << '\n';
            }
        }
    }

    destroyStream (stream);
    return true;
}

void CSMSettings::SettingManager::mergeSettings(DefinitionPageMap &destMap, DefinitionPageMap &srcMap,
                                              MergeMethod mergeMethod)
{
    if (srcMap.size() == 0)
        return;

    //merge srcMap values into destMap according to the specified merge method.
    //Merge method indcates action taken on DefinitionMap objects which share the
    //same key in both maps.  It does not discriminate between duplicate values
    //found within the two SettingMaps.
    //
    //Merge_Accept - add all values in the DefinitionMap in srcMap to destMap
    //Merge_Ignore - skip all SettingMaps in srcMap which share a key with
    //               DefinitionMap in destMap.
    //Merge_Overwrite - replace all DefinitionMap objects in dest map with the
    //                 DefinitionMap objects in srcMap which share the same keys


    //iterate each page in the source map
    foreach (const QString &pageKey, srcMap.keys())
    {
        DefinitionMap *srcSetting = srcMap.value(pageKey);

        //Unique Page:
        //insertfrom the source map
        if (!destMap.keys().contains (pageKey))
        {
            destMap.insert (pageKey, srcSetting);
            continue;
        }

        //Duplicate Page:
        //iterate the settings in the source and check for duplicates in the
        //destination
        foreach (const QString &srcKey, srcSetting->keys())
        {
            DefinitionMap *destSetting = destMap.value (pageKey);

            //Unique Setting:
            //insert and continue
            if (!destSetting->keys().contains(srcKey))
            {
                destMap.insert(srcKey, srcMap.value(pageKey));
                continue;
            }

            //Duplicate Setting
            //depending on the merge method, either replace the existing
            //setting map in destination with the corresponding map in source
            //or merge the values of both setting maps into destination.
            switch (mergeMethod)
            {
            case Merge_Overwrite:
                destSetting->insert(srcKey, srcSetting->value(srcKey));
            break;

            case Merge_Accept:
                foreach (const QString &value, *(srcSetting->value(srcKey)))
                    destSetting->value(srcKey)->append(value);
            break;

            default:    //Merge_Ignore is do-nothing
            break;
            }
        }
    }
}

QTextStream *CSMSettings::SettingManager::openFilestream (const QString &filePath,
                                                        bool isReadOnly) const
{
    QIODevice::OpenMode openFlags = QIODevice::Text;

    if (isReadOnly)
        openFlags = QIODevice::ReadOnly | openFlags;
    else
        openFlags = QIODevice::ReadWrite | QIODevice::Truncate | openFlags;

    QFile *file = new QFile(filePath);
    QTextStream *stream = 0;

    if (file->open(openFlags))
        stream = new QTextStream(file);

    if (stream)
        stream->setCodec(QTextCodec::codecForName("UTF-8"));

    return stream;
}

void CSMSettings::SettingManager::destroyStream(QTextStream *stream) const
{
    stream->device()->close();

    delete stream;
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

void CSMSettings::SettingManager::addDefinitions (DefinitionPageMap &pageMap)
{
    foreach (QString pageName, pageMap.keys())
    {
        DefinitionMap *settingMap = pageMap.value (pageName);

        foreach (QString settingName, (*settingMap).keys())
        {
            QStringList *values = settingMap->value (settingName);
            Setting *setting = findSetting (pageName, settingName);

            if (!setting)
            {
                qWarning() << "Found definitions for undeclared setting "
                           << pageName << "." << settingName;
                continue;
            }

            if (values->size() == 0)
                values->append (setting->defaultValues());

            setting->setDefinedValues (*values);
        }
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
        if (setting->name() == settingName)
        {
            if (setting->page() == pageName)
                return setting;
        }
    }
    return 0;
}

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

CSMSettings::SettingPageMap CSMSettings::SettingManager::settingPageMap() const
{
    SettingPageMap pageMap;

    foreach (Setting *setting, mSettings)
        pageMap[setting->page()].append (setting);

    return pageMap;
}
