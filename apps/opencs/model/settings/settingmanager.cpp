#include <QFile>
#include <QTextCodec>
#include <QMessageBox>
#include <QDebug>
#include <QList>

#include "setting.hpp"
#include "settingmanager.hpp"
#include "settingfiltermodel.hpp"

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

void CSMSettings::SettingManager::addSetting (Setting *setting)
{
    if (!setting)
        return;

    //get list of all settings for the current setting name
    Setting *exSetting = findSetting (setting->page(), setting->name());

    if (exSetting)
    {
        qWarning() << "Duplicate declaration encountered: "
                   << setting->page() << '.' << setting->name();
        return;
    }

    //add declaration to the model
    mSettings.append (setting);
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

bool CSMSettings::SettingManager::writeFilestream(QTextStream *stream)
{
    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return false;
    }

    foreach (const QString &page, mSelectors.keys())
    {
        QList <SelectorPair> list = mSelectors.value (page);

        if (list.size() == 0)
            continue;

        *stream << "[" << page << "]" << "\n";

        foreach (const SelectorPair &pair, list)
        {
            //skip settings that are proxy masters
            if (pair.second->proxyIndex() == 0)
                continue;

            QStringList values = pair.second->selectionToStringList();

            foreach (const QString &value, values)
                *stream << pair.first << " = " << value << "\n";
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

CSMSettings::SettingList CSMSettings::SettingManager::
                                        findSettings(const QStringList &list)
{
    SettingList settings;

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

CSMSettings::SettingList CSMSettings::SettingManager::findSettings
                                                    (const QString &pageName)
{
    SettingList settings;

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

CSMSettings::Selector *CSMSettings::SettingManager::findSelector
                        (const QString &pageName, const QString &settingName)
{
    if (!mSelectors.keys().contains(pageName))
        return 0;

    QList <SelectorPair> list = mSelectors[pageName];
    foreach (const SelectorPair &pair, list)
    {
        if (pair.first == settingName)
            return pair.second;
    }

    return 0;
}

CSMSettings::Selector * CSMSettings::SettingManager::createSelector
                                                (CSMSettings::Setting *setting)
{
    qDebug() << "Creating selector for setting " << setting->name();

    QStringList definitions = setting->definedValues();
    Selector *selector = 0;

    qDebug() << "defns: " << setting->definedValues();
    qDebug() << "deflts" << setting->defaultValues();

    //add default value if no definitions are found
    if (definitions.size() == 0)
    {
        definitions.append (setting->defaultValues());
        setting->setDefinedValues (definitions);
    }

    // use the declared values as the model and the definitions for selection,
    // if there are declared values.  Otherwise, use the definitions as the
    // model and select everything
    if (setting->declaredValues().size() > 0)
        selector = new Selector (setting->name(), setting->declaredValues());
    else
    {
        selector = new Selector (setting->name(), definitions);
        selector->setSelectAll();
    }

    selector->setViewSelection (definitions);

    mSelectors[setting->page()].append( SelectorPair
                                                (setting->name(), selector));

    return selector;
}



CSMSettings::Selector *CSMSettings::SettingManager::selector
                        (const QString &pageName, const QString &settingName)
{
    Selector *selector = findSelector (pageName, settingName);

    if (selector)
        return selector;

    Setting *setting = findSetting (pageName, settingName);

    if (!setting)
        return 0;

    return createSelector (setting);
}
