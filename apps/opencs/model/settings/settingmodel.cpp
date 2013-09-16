#include "settingmodel.hpp"

#include <QDebug>
int CSMSettings::SettingModel::sColumnCount = 7;

CSMSettings::SettingModel::SettingModel(QObject *parent) :
    QAbstractItemModel(parent)
{}

CSMSettings::SettingModel::~SettingModel()
{
    while (mSettings.size() > 0)
    {
        Setting *setting = mSettings.first();
        mSettings.removeFirst();
        delete setting;
    }
}

int CSMSettings::SettingModel::columnCount(const QModelIndex &parent) const
{
    return sColumnCount;
}

int CSMSettings::SettingModel::rowCount(const QModelIndex &parent) const
{
    return mSettings.size();
}

QVariant CSMSettings::SettingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Setting *setting = 0;
    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        setting = mSettings.at(index.row());

        return setting->item(index.column());
        break;

    case Qt::UserRole:

        return Setting::sColumnNames.at(index.column());

    default:

        break;
    }

    return QVariant();
}

bool CSMSettings::SettingModel::setDataByName
    (const QString &sectionName, const QString &settingName, const QVariant &value)
{
    bool success = false;

    for (int i = 0; i < rowCount(); ++i)
    {
        Setting *setting = mSettings.at(i);

        if (setting->sectionName() == sectionName)
        {
            if (setting->name() == settingName)
            {
                success = true;
                setData (index(i, 0, QModelIndex()), value);
                break;
            }
        }
    }
    return success;
}

bool CSMSettings::SettingModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    Setting *setting = mSettings.at(index.row());
    QString qValue = value.toString();

    if (setting->valueList().size() > 0)
    {
        if (!(setting->valueList().contains(qValue)))
            return false;
    }

    setting->setValue (qValue);

    emit dataChanged(index, index);

    return true;
}

Qt::ItemFlags CSMSettings::SettingModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (index.column() == 2)
        return defaultFlags | Qt::ItemIsEditable;

    return defaultFlags;
}

CSMSettings::Setting *CSMSettings::SettingModel::createSetting(const QString &name,
                                                               const QString &section,
                                                               const QString &defaultValue)
{
    Setting *setting = new Setting (name, section, defaultValue);

    beginInsertRows(QModelIndex(), mSettings.size(), mSettings.size());
    {
        mSettings.append(setting);
    } endInsertRows();

    emit dataChanged(index(mSettings.size() - 1, 0, QModelIndex()), index(mSettings.size() - 1, 0, QModelIndex()));

    return setting;
}

const CSMSettings::Setting *CSMSettings::SettingModel::getSetting (int row) const
{
    if (row < mSettings.size())
        return mSettings.at(row);

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
