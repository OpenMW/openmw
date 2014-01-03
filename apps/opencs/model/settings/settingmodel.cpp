#include "settingmodel.hpp"

#include <QDebug>

CSMSettings::SettingModel::SettingModel(QObject *parent) :
    QAbstractItemModel(parent)
{}

int CSMSettings::SettingModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

int CSMSettings::SettingModel::rowCount(const QModelIndex &parent) const
{
    return mSettingValues.size();
}

QVariant CSMSettings::SettingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SectionSettingPair dataPair = mSettingValues.at(index.row());
    QString key = dataPair.first + "." + dataPair.second.first;

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (index.column())
        {
        case 0:
            return dataPair.second.first;
        break;

        case 1:
            return dataPair.first;
        break;

        case 2:
            return dataPair.second.second;
        break;

        case 3:
            return mSettings.value(key)->valueList();
        break;
        case 4:
            return key;
            break;

        default:
        break;
        }

    default:
    break;
    }

    return QVariant();
}

bool CSMSettings::SettingModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

    int row = index.row();

    if (!index.isValid())
        return false;

    if (row > rowCount())
            return false;

    SectionSettingPair dataPair = mSettingValues.at(row);
    dataPair.second.second = value.toString();

    mSettingValues.replace(row, dataPair);

    emit dataChanged(index, index);

    return true;
}

Qt::ItemFlags CSMSettings::SettingModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (index.column() == 1)
        return defaultFlags | Qt::ItemIsEditable;

    return defaultFlags;
}

//a created setting is a declared setting
//setting definitions occur later in the model creation process
//declared settings are accessed by key section.setting
//defined settings are accessed by index
//after loading setting definitions, setting defaults need to be applied
//where no definitions are found for a corresponding declaration.

CSMSettings::Setting *CSMSettings::SettingModel::declareSetting (
                                        const QString &settingName,
                                        const QString &settingSection,
                                        const QString &defaultValue)
{
    Setting *setting = new Setting(settingName, settingSection, defaultValue, this);
    mSettings[settingSection + "." + settingName] = setting;
    return setting;
}

QModelIndex CSMSettings::SettingModel::defineSetting (
                                            const QString &settingName,
                                            const QString &sectionName,
                                            const QString &value)
{
    SectionSettingPair dataPair;

    dataPair.first = sectionName;
    dataPair.second.first = settingName;
    dataPair.second.second = value;

    int settingRow = rowCount();

    insertRow(settingRow);
    mSettingValues.replace(settingRow, dataPair);

    QModelIndex idx = index(settingRow, 0, QModelIndex());

    emit layoutChanged();

    return idx;
}

QString CSMSettings::SettingModel::getSettingValue (int row) const
{
    if (row < mSettingValues.size())
        return mSettingValues.at(row).second.second;

    return QString("");
}

QString CSMSettings::SettingModel::getSettingValue (const QString &sectionName,
                                               const QString &settingName) const
{
    foreach (const SectionSettingPair &dataPair, mSettingValues)
    {
        if (dataPair.first == sectionName)
            if (dataPair.second.first == settingName)
                return dataPair.second.second;
    }

    return QString("");
}

QModelIndex CSMSettings::SettingModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if ((row >= 0 && row < rowCount()) && (column >= 0 && column < columnCount()))
        return createIndex (row, column);

    return QModelIndex();
}

bool CSMSettings::SettingModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > rowCount())
        return false;

    SectionSettingPair ssPair;
    SettingValuePair svPair;

    ssPair.second = svPair;

    for (int i = row; i < row + count; i++)
    {
        beginInsertRows(parent, i, i);
        {
            mSettingValues.insert(row, ssPair);
        } endInsertRows();
    }

    return true;
}

bool CSMSettings::SettingModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= rowCount())
        return false;

    for (int i = row; i < row + count; i++)
    {
        beginRemoveRows (parent, i, i);
        {
            mSettingValues.removeAt(row);
        } endRemoveRows();
    }

    emit layoutChanged();
    qDebug() << "CSMSettings::SettingModel::removeRows() complete";
    return true;
}

QStringList CSMSettings::SettingModel::getSettingValueList
                                        (const QString &sectionName,
                                            const QString &settingName) const
{
    QString key = sectionName + "." + settingName;

    if (mSettings.keys().contains(key))
        return mSettings.value(key)->valueList();

    return QStringList();
}

CSMSettings::SectionSettingPair CSMSettings::SettingModel::
                                        getSectionSettingPair(int index) const
{
    if (index >= 0 && index < rowCount())
        return mSettingValues.at(index);

    return SectionSettingPair();
}

bool CSMSettings::SettingModel::hasDeclaredSetting(const QString &key) const
{
    return mSettings.keys().contains(key);
}

void CSMSettings::SettingModel::validate()
{
    QStringList settingKeys = mSettings.keys();

    //iterate each declared setting, verifying:
    //1. All definitions match against the value list (if defined)
    //2. Undefined settings are given the default value as a definition (if defined)
    foreach (const QString &key, settingKeys)
    {
        Setting *setting = mSettings.value(key);

        QString sectionName = setting->section();
        QString settingName = setting->name();

        bool isUndefined = true;
        bool hasValueList = setting->valueList().size() > 0;

        SettingDefinitionList::Iterator it = mSettingValues.begin();

        while (it != mSettingValues.end())
        {
            if ((*it).first == sectionName)
            {
                if ((*it).second.first == settingName)
                {
                    //indicate the setting has values
                    //to avoid adding a default value later
                    if (isUndefined)
                        isUndefined = false;

                    //if the valuelist is non-zero, ensure all loaded
                    //values are found in the list.  Delete the ones that
                    //aren't found from the model.
                    if (hasValueList)
                    {

                        if (!setting->valueList().contains((*it).second.second))
                            it = mSettingValues.erase(it, it);
                    }
                }
            }
            it++;
        }

        if (isUndefined)
        {
            if (setting->defaultValue() != "")
                defineSetting (settingName, sectionName, setting->defaultValue());
        }
    }
}
