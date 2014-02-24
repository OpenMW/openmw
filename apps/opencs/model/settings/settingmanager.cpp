#include <QFile>
#include <QTextCodec>
#include <QMessageBox>
#include <QDebug>
#include <QList>
#include <QStandardItem>

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

void CSMSettings::SettingManager::addDefinition (const QString &settingName,
                                                 const QString &pageName,
                                                 const QString &value)
{
    RowItemList *settingItems = findSetting (settingName, pageName);

    if (!settingItems)
    {
        qWarning() << "Definition found for undelcared setting "
                  << pageName << '.' << settingName;
        return;
    }

    //find the definitions list under the first item of the setting row
    QStandardItem *nameItem = settingItems->at (Property_Name);
    QStandardItem *definitions = nameItem->child (PropertyList_DefinedValues);
    QStringList values = definitions->data (Qt::UserRole).toStringList();

    values.append (value);

    definitions->setData (values, Qt::UserRole);

    definitions->removeColumn (0);
    definitions->appendColumn (buildItemList (values));
}

QList <QStandardItem *> CSMSettings::SettingManager::buildItemList
                                                (const QStringList &list) const
{
    QList <QStandardItem *> itemList;

    foreach (const QString &item, list)
        itemList.append (new QStandardItem (item));

    return itemList;
}
void CSMSettings::SettingManager::addDeclaration (Setting *setting)
{
    if (!setting)
        return;

    //get list of all settings for the current setting name
    RowList *rows = findSettings (setting->name(), Property_Name);

    //if there are multiple records with the same name, filter by page name
    if (rows->size() > 1)
        rows = findSettings (rows, setting->page(), Property_Page);

    //if settings share name and page, abort.  Duplicate setting found.
    if (rows->size() > 0)
    {
        qWarning() << "Duplicate declaration encountered: "
                   << setting->page() << '.' << setting->name();
        return;
    }

    //add declaration to t1he model
    mSettingModel.appendRow (setting->rowList());
}

CSMSettings::PageMap
                CSMSettings::SettingManager::readFilestream (QTextStream *stream)
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

bool CSMSettings::SettingManager::writeFilestream(QTextStream *stream)
{
    if (!stream)
    {
        displayFileErrorMessage(mReadWriteMessage, false);
        return false;
    }

    WriterSortModel sortModel;

    sortModel.setSourceModel (&mSettingModel);
    sortModel.sort(0);

    QString currPage = "";

    for (int i = 0; i < sortModel.rowCount(); i++)
    {
        QModelIndex name_idx = sortModel.index (i, Property_Name);
        QModelIndex page_idx = sortModel.index (i, Property_Page);
        QModelIndex value_idx = sortModel.index
                                        (i, static_cast<SettingProperty>(2));

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

void CSMSettings::SettingManager::mergeSettings(PageMap &destMap, PageMap &srcMap,
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
    //               SettingMap in destMap.
    //Merge_Overwrite - replace all SettingMap objects in dest map with the
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

void CSMSettings::SettingManager::buildModel (PageMap &pageMap)
{
    foreach (const QString &pageName, pageMap.keys())
    {
        SettingMap *settingMap = pageMap.value (pageName);

        foreach (const QString &settingName, (*settingMap).keys())
        {
            QStringList *values = settingMap->value (settingName);

            foreach (const QString &value, *values)
               addDefinition (settingName, pageName, value);
        }
    }
}

void CSMSettings::SettingManager::validate (PageMap &pageMap)
{
    //iterate each declared setting, verifying that every defined item
    //matches a value in the value list, if one exists.

    foreach (const QString &pageName, pageMap.keys())
    {
        SettingMap *settingMap = pageMap.value (pageName);

        foreach (const QString &settingName, settingMap->keys())
        {
            RowItemList *settingItems = findSetting (settingName, pageName);

            //skip if the setting was not found
            if (settingItems->size() == 0)
            {
                qWarning() << "Definition found for undeclared setting";
                continue;
            }

            QStandardItem *nameItem = settingItems->at(Property_Name);
            QStandardItem *valueListItem = nameItem->child
                                                (PropertyList_DeclaredValues);

            //skip if no declared values exist
            if (!valueListItem->hasChildren())
                continue;

            QStringList valueList =
                            valueListItem->data (Qt::UserRole).toStringList();

            QStringList *definedValues = settingMap->value (settingName);

            //iterate all the values defined for this setting, checking against
            //the value list.  Remove values not found in the list
            QStringList::Iterator it = definedValues->begin();

            while (it != definedValues->end())
            {
                if (!valueList.contains (*it))
                {
                    qWarning() << "invalid setting definition found: " << *it;
                    it = valueList.erase (it);
                }
                else
                    it++;
            }

        }
    }
}

CSMSettings::RowList *CSMSettings::SettingManager::findSettings
                (CSMSettings::RowList *source, const QString &text, int column)
{
    RowList *list = 0;

    //pull from entire model if no source is provided
    if (!source)
        return findSettings (text, column);

    //iterate each row in the list, looking for rows matching the search
    //critereon
    foreach (RowItemList *itemList, *source)
    {
        QStandardItem *item = itemList->at(column);

        if (item->data().toString()==text)
            list->append (itemList);
    }

    return list;
}

CSMSettings::RowList *CSMSettings::SettingManager::findSettings
                (const QString &text, int column)
{
    RowList *retList = new RowList();

    //Use the QStandardItem model's findItems() function to return a list of
    //matching items.  Then, iterate the list and append the entire row to the
    //return row list.
    RowItemList itemList = mSettingModel.findItems
                                            (text, Qt::MatchExactly, column);

    foreach (QStandardItem *item, itemList)
        retList->append (findSetting (item->row()));

    return retList;
}

CSMSettings::RowItemList *CSMSettings::SettingManager::findSetting
                        (const QString &settingName, const QString &pageName)
{
    RowList *settingRows = findSettings (settingName, Property_Name);

    //if multiple settings found, filter list by page name
    if (settingRows->size() > 1)
        settingRows = findSettings (settingRows, pageName, Property_Page);

    if (settingRows->size() > 1)
        qWarning() << "multiple declarations for setting "
                   << pageName << '.' << settingName << " found.";

    if (settingRows->size() < 1)
    {
        qWarning() << "setting "
                   << pageName << '.' << settingName << " not found.";
        return 0;
    }

    if (settingRows->at(0)->at(Property_Page)->text() != pageName)
    {
        qWarning() << "setting "
                   << pageName << '.' << settingName << " not found.";
        return 0;
    }

    return settingRows->at(0);
}

CSMSettings::RowItemList *CSMSettings::SettingManager::findSetting
                                                                (int row) const
{
    if (!(row > -1 && row < mSettingModel.rowCount()))
        return 0;

    RowItemList *rowItems = new RowItemList();

    for (int i = 0; i < sSettingPropertyCount; i++)
        rowItems->append(mSettingModel.item(row, i));

    return rowItems;
}

CSMSettings::Setting CSMSettings::SettingManager::getSetting
                        (const QString &pageName, const QString &settingName)
{
    RowItemList *settingRow = findSetting (settingName, pageName);

    Setting theSetting;
    int i = 0;

     foreach (QStandardItem *item, *settingRow)
        theSetting.setProperty(i++, item);

    return theSetting;
}

QList <CSMSettings::Setting> CSMSettings::SettingManager::getSettings
                                                    (const QString &pageName)
{
    RowList *settingRows = findSettings (pageName, Property_Page);
    QList <CSMSettings::Setting> settingList;

    foreach (RowItemList *items, *settingRows)
    {
        QString settingName =
                    items->at(Property_Name)->data (Qt::DisplayRole).toString();

        settingList.append (getSetting (pageName, settingName));
    }

    return settingList;
}
