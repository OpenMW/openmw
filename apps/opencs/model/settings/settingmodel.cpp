#include "settingmodel.hpp"

#include <QDebug>

CSMSettings::SettingModel::SettingModel(QObject *parent) :
    QAbstractItemModel(parent)
{}

int CSMSettings::SettingModel::columnCount(const QModelIndex &parent) const
{
    return 4;
}

int CSMSettings::SettingModel::rowCount(const QModelIndex &parent) const
{
    return mSettings.size();
}

QVariant CSMSettings::SettingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SettingData *setting = mSettings.at(index.row());

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (index.column())
        {
        case 0:
            return setting->name();
        break;

        case 1:
            return setting->section();
        break;

        case 2:
            return setting->value();
        break;

        case 3:
            return setting->valueList();

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

    if (!index.isValid())
        return false;

    if (index.row() > mSettings.count())
            return false;

    mSettings.at(index.row())->setValue (value.toString());

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

CSMSettings::SettingData *CSMSettings::SettingModel::createSetting (
                                            const QString &name,
                                            const QString &section,
                                            const QString &value,
                                            const QStringList &valueList)
{
    SettingData *setting = new SettingData(section, name, valueList, this);
    setting->setValue(value);

    int settingRow = rowCount();

    insertRow(settingRow);
    mSettings.replace(settingRow, setting);

    QModelIndex idx = index(settingRow, 0, QModelIndex());

    emit dataChanged(idx, idx);

    return setting;
}

const CSMSettings::SettingData *CSMSettings::SettingModel::getSetting (int row) const
{
    if (row < mSettings.size())
        return mSettings.at(row);

    return 0;
}

CSMSettings::SettingData *CSMSettings::SettingModel::getSetting (const QString &sectionName,
                                                                   const QString &settingName)
{
    foreach (CSMSettings::SettingData *setting, mSettings)
    {
        if (setting->section() == sectionName)
            if (setting->name() == settingName)
                return setting;
    }

    return 0;
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
    if (row > mSettings.size())
        return false;

    for (int i = row; i < row + count; i++)
        mSettings.insert(row, new SettingData("", "", QStringList(),this));

    emit layoutChanged();

    return true;
}

bool CSMSettings::SettingModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= mSettings.count())
        return false;

    for (int i = row; i < row + count; i++)
        mSettings.removeAt(row);

    emit layoutChanged();

    return true;
}
