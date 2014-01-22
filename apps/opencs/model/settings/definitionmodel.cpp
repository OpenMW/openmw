#include "definitionmodel.hpp"
#include "../../view/settings/support.hpp"

#include <QDebug>

CSMSettings::DefinitionModel::DefinitionModel(QObject *parent) :
    QAbstractItemModel(parent)
{}

int CSMSettings::DefinitionModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

int CSMSettings::DefinitionModel::rowCount(const QModelIndex &parent) const
{
    return mDefinitions.size();
}

QVariant CSMSettings::DefinitionModel::data(const QModelIndex &idx,
                                            int role) const
{
    if (!idx.isValid())
        return QVariant();

    DefinitionListItem dataPair = mDefinitions.at(idx.row());

    SettingColumn columnEnum = static_cast<SettingColumn>(idx.column());

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (columnEnum)
        {
        case Setting_Name:
            return dataPair.second.first;
        break;

        case Setting_Page:
            return dataPair.first;
        break;

        case Setting_Value:
            return dataPair.second.second;
        break;

        default:
        break;
        }

    default:
    break;
    }

    return QVariant();
}

bool CSMSettings::DefinitionModel::setData(const QModelIndex &idx,
                                           const QVariant &value, int role)
{
    int row = idx.row();

    if (!idx.isValid())
        return false;

    if (row > rowCount())
        return false;

    DefinitionListItem dataPair = mDefinitions.at(row);

    dataPair.second.second = value.toString();

    mDefinitions.replace(row, dataPair);

    emit dataChanged(idx, idx);

    return true;
}

Qt::ItemFlags CSMSettings::DefinitionModel::flags
                                            (const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (static_cast<SettingColumn>(index.column()) == Setting_Value)
        return defaultFlags | Qt::ItemIsEditable;

    return defaultFlags;
}

QModelIndex CSMSettings::DefinitionModel::defineSetting (
                                            const QString &settingName,
                                            const QString &pageName,
                                            const QString &value)
{
    DefinitionListItem dataPair;

    dataPair.first = pageName;
    dataPair.second.first = settingName;
    dataPair.second.second = value;

    int settingRow = rowCount();

    insertRow (settingRow);
    mDefinitions.replace (settingRow, dataPair);

    QModelIndex idx = index (settingRow,
                            static_cast<int>(Setting_Name),
                            QModelIndex());

    emit layoutChanged();

    return idx;
}

QModelIndex CSMSettings::DefinitionModel::index(int row, int column,
                                             const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if ((row >= 0 && row < rowCount()) &&
        (column >= 0 && column < columnCount()))
    {
            return createIndex (row, column);
    }

    return QModelIndex();
}

bool CSMSettings::DefinitionModel::insertRows(int row, int count,
                                                    const QModelIndex &parent)
{
    if (row > rowCount())
        return false;

    DefinitionListItem ssPair;
    DefinitionPair svPair;

    ssPair.second = svPair;

    for (int i = row; i < row + count; i++)
    {
        beginInsertRows(parent, i, i);
        {
            mDefinitions.insert(row, ssPair);
        } endInsertRows();
    }

    return true;
}

bool CSMSettings::DefinitionModel::removeRows(int row, int count,
                                           const QModelIndex &parent)
{
    if (row >= rowCount())
        return false;

    for (int i = row; i < row + count; i++)
    {
        beginRemoveRows (parent, i, i);
        {
            mDefinitions.removeAt(row);
        } endRemoveRows();
    }

    emit layoutChanged();

    return true;
}

CSMSettings::DefinitionListItem CSMSettings::DefinitionModel
                                                    ::getItem (int row) const
{
    if (row >= 0 && row < mDefinitions.size())
        return mDefinitions.at(row);

    return DefinitionListItem();
}
