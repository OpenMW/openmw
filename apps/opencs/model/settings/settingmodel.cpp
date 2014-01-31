#include "settingmodel.hpp"
#include <QFile>
#include <QTextCodec>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include <QDebug>

CSMSettings::SettingModel::SettingModel(QObject *parent) :
    QObject(parent)
{
    mReadWriteMessage = QObject::tr("<br><b>Could not open or create file for \
                        writing</b><br><br> Please make sure you have the right\
                         permissions and try again.<br>");

    mReadOnlyMessage = QObject::tr("<br><b>Could not open file for \
                        reading</b><br><br> Please make sure you have the \
                        right permissions and try again.<br>");
}

void CSMSettings::SettingModel::removeUndeclaredDefinitions (PageMap &pageMap)
{
    //iterate each definition, ensuring that there is a corresponding
    //declaration.  Delete unmatched definitions (page / name match only).

    int pageRow = 0;

    QSortFilterProxyModel declaredPages (this);
    declaredPages.setSourceModel (&mDeclarationModel);
    declaredPages.setFilterKeyColumn (Setting_Page);

    QSortFilterProxyModel declaredSettings (this);
    declaredSettings.setSourceModel (&declaredPages);
    declaredSettings.setFilterKeyColumn (Setting_Name);

    while (pageRow != pageMap.size())
    {
        QString pageName = pageMap.keys().at(pageRow);

        declaredPages.setFilterRegExp (pageName);

        if (declaredPages.rowCount() == 0)
        {
            pageMap.remove (pageName);
            continue;
        }

        int settingRow = 0;

        SettingMap *settingMap = pageMap.value (pageName);

        while (settingRow != settingMap->size())
        {
            QString settingName = settingMap->keys().at(settingRow);

            declaredSettings.setFilterRegExp (settingName);

            if (declaredSettings.rowCount() == 0)
            {
                settingMap->remove (settingName);
                continue;
            }

            settingRow++;
        }

        pageRow++;
    }
}

void CSMSettings::SettingModel::validateDefinitions (PageMap &pageMap)
{
    //iterate each declared setting, verifying:
    //1. All definitions match against the value list, if provided
    //2. Undefined settings are given the default value as a definition

    for (int i = 0; i < mDeclarationModel.rowCount(); i++)
    {
        Setting *setting = mDeclarationModel.getSetting (i);

        SettingMap *settingMap = 0;

        //create missing pages
        if (!(pageMap.keys().contains(setting->pageName)))
            pageMap[setting->pageName] = new SettingMap();

        settingMap = pageMap.value(setting->pageName);

        //create missing settings
        if (!(settingMap->keys().contains(setting->settingName)))
            settingMap->operator[](setting->settingName) = new QStringList;

        QStringList *settingValues = settingMap->value(setting->settingName);

        if (settingValues->size() == 0)
            settingValues->append (setting->defaultValue);

        //iterate the value list and remove any invalid values from
        //the defined settings
        if (setting->valueList.size() > 0)
        {
            QStringList::Iterator it = settingValues->begin();

            while (it != settingValues->end())
            {
                if (setting->valueList.contains(*it))
                    it++;
                else
                    it = settingValues->erase(it);
            }
        }
    }
}

void CSMSettings::SettingModel::validate (PageMap &pageMap)
{
    removeUndeclaredDefinitions (pageMap);
    validateDefinitions (pageMap);
}

CSMSettings::PageMap
                CSMSettings::SettingModel::readFilestream (QTextStream *stream)
{
    //regEx's for page names and keys / values
    QRegExp pageRegEx ("^\\[([^]]+)\\]");
    QRegExp keyRegEx ("^([^=]+)\\s*=\\s*(.+)$");

    QString currPage = "Unassigned";

    PageMap pageMap;

    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return pageMap;
    }

    if (stream->atEnd())
        return pageMap;

    SettingMap *settingMap = new SettingMap();
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
            settingMap = new SettingMap();
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

void CSMSettings::SettingModel::mergeSettings(PageMap &destMap, PageMap &srcMap,
                                              MergeMethod mergeMethod)
{
    if (srcMap.size() == 0)
        return;

    //merge srcMap values into destMap according to the specified merge method.
    //Merge method indcates action taken on SettingMap objects which share the
    //same key in both maps.  It does not discriminate between duplicate values
    //found within the two SettingMaps.
    //
    //Merge_Accept - add all values in the SettingMap in srcMap to destMap
    //Merge_Ignore - skip all SettingMaps in srcMap which share a key with
    //               a destMap SettingMap.
    //MergeOverwrite - replace all SettingMap objects in dest map with the
    //                 SettingMap objects in srcMap which share the same keys


    //iterate each page in the source map
    foreach (const QString &pageKey, srcMap.keys())
    {
        SettingMap *srcSetting = srcMap.value(pageKey);

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
            SettingMap *destSetting = destMap.value (pageKey);

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

QTextStream *CSMSettings::SettingModel::openFilestream (const QString &filePath,
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

void CSMSettings::SettingModel::destroyStream(QTextStream *stream) const
{
    stream->device()->close();

    delete stream;
}

bool CSMSettings::SettingModel::writeFilestream(QTextStream *stream)
{
    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return false;
    }

    WriterSortModel sortModel;

    sortModel.setSourceModel (&mDefinitionModel);
    sortModel.sort(0);

    QString currPage = "";

    for (int i = 0; i < sortModel.rowCount(); i++)
    {
        QModelIndex name_idx = sortModel.index (i, Setting_Name);
        QModelIndex page_idx = sortModel.index (i, Setting_Page);
        QModelIndex value_idx = sortModel.index (i, Setting_Value);

        QString pageName = sortModel.data (page_idx).toString();
        QString settingName = sortModel.data (name_idx).toString();
        QString value = sortModel.data (value_idx).toString();

        if (currPage != pageName)
        {
            currPage = pageName;
            *stream << "[" << currPage << "]" << "\n";
        }

        *stream << settingName << " = "
                << value << "\n";
    }

    destroyStream (stream);
    return true;
}

void CSMSettings::SettingModel::displayFileErrorMessage(const QString &message,
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

void CSMSettings::SettingModel::buildModel (PageMap &pageMap)
{
    foreach (const QString &pageName, pageMap.keys())
    {
        SettingMap *settingMap = pageMap.value (pageName);

        foreach (const QString &settingName, (*settingMap).keys())
        {
            QStringList *values = settingMap->value (settingName);

            foreach (const QString &value, *values)
                mDefinitionModel.defineSetting (settingName, pageName, value);
        }
    }
}
