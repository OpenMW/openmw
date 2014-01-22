#include <QStringList>

#include "definitionmodel.hpp"
#include "booleanadapter.hpp"
#include "setting.hpp"

#include <QDebug>
CSMSettings::BooleanAdapter::BooleanAdapter (DefinitionModel &model,
                                            const CSMSettings::Setting *setting,
                                            QObject *parent)

    : ViewAdapter (model, setting->pageName, setting->settingName, parent)
{

    //create a list of QString pairs which represent the setting values and
    //whether or not they are set (true / false)

    setObjectName (settingName() + "_adapter");
    foreach (const QString &listValue, setting->valueList)
    {
        QPair<QString, QBool *> settingPair(listValue, new QBool(true));

        for (int i = 0; i < filter()->rowCount(); i++)
        {
            QString modelValue = filter()->data(
                                filter()->index (i, Setting_Value)).toString();

            if (modelValue == listValue)
            {
                settingPair.second = new QBool(true);
                break;
            }
        }
        mSettings.append(settingPair);
    }

    connect (filter(),
             SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
             this,
             SLOT(slotDataChanged(const QModelIndex &, const QModelIndex &)));

    connect (filter(), SIGNAL(layoutChanged()),
             this, SLOT(slotUpdateData()));
}

void CSMSettings::BooleanAdapter::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &botRight)
{
}

void CSMSettings::BooleanAdapter::slotUpdateData()
{
    for (int i = 0; i < mSettings.count(); i++)
    {
        QPair <QString, QBool *> data = mSettings.at(i);

        QString settingValue = data.first;

        bool settingFound = false;

        for (int j = 0; j < filter()->rowCount(); j++)
        {
            QString filterValue = filter()->data (
                        filter()->index(j, Setting_Value)
                        ).toString();

            settingFound = ( filterValue  == settingValue);

            if (settingFound)
            {
                if ((*data.second) == QBool(false))
                    data.second = new QBool(true);
                break;
            }
        }

        if (!settingFound)
        {
            if (*data.second == QBool(true))
                data.second = new QBool (false);
        }
        mSettings.replace(i, data);
        QModelIndex idx = index(i, BooleanSetting_ValueState, QModelIndex());
        emit dataChanged (idx, idx);
    }
}

QModelIndex CSMSettings::BooleanAdapter::valueIndex (const QString &value,
                                                     SettingColumn column)
{
    for (int i = 0; i < filter()->rowCount(); i++)
    {
        QModelIndex idx = filter()->index(i, column);

        if (filter()->data(idx).toString() == value)
            return idx;
    }

    return QModelIndex();
}

bool CSMSettings::BooleanAdapter::valueExists (const QString &value) const
{
    bool success = false;

    for (int i = 0; i< mSettings.size(); i++)
    {
        success = (mSettings.at(i).first == value);

        if (success)
            break;
    }

    return success;
}

bool CSMSettings::BooleanAdapter::insertValue(const QString &value)
{
    //if the item isn't found in the local model, abort
    if (!valueExists (value))
        return false;

    return ViewAdapter::insertValue (value);
}

bool CSMSettings::BooleanAdapter::removeValue (const QString &value)
{
    //if the item isn't found in the local model, abort
    if (!valueExists (value))
        return false;

     return ViewAdapter::removeValue (value);
}

QVariant CSMSettings::BooleanAdapter::data(const QModelIndex &index, int role) const
{
    if (!validIndex(index))
        return QVariant();

    SettingColumn column = static_cast<SettingColumn> (index.column());

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        switch (column)
        {
        case BooleanSetting_Value:
            return mSettings.at(index.row()).first;
        break;

        case BooleanSetting_ValueState: // (true / false)
            return QVariant(*(mSettings.at(index.row()).second));
        break;

        default:
        break;
        }

    default:
    break;
    }

    return QVariant();
}

int CSMSettings::BooleanAdapter::rowCount(const QModelIndex &parent) const
{
    return mSettings.size();
}

int CSMSettings::BooleanAdapter::columnCount(const QModelIndex &parent) const
{
    return 2;
}

bool CSMSettings::BooleanAdapter::validIndex(QModelIndex idx) const
{
    if (!idx.isValid())
        return false;

    if (idx.row() < 0 || idx.row() >= rowCount())
        return false;

    return true;
}

bool CSMSettings::BooleanAdapter::setData(const QModelIndex &index,
                                          const QVariant &value, int role)
{
    if (!validIndex(index))
        return false;

    QString item = mSettings.at(index.row()).first;

    if (!setSourceValue (value.toBool(), item))
        return false;

    *(mSettings.at(index.row()).second) = QBool(value.toBool());

    emit dataChanged(index, index);

    return true;
}

bool CSMSettings::BooleanAdapter::setSourceValue (bool state,
                                                 const QString &value)
{
    if (singleValueMode())
        return setSingleValue(value);

    return setMultiValue(state, value);
}

bool CSMSettings::BooleanAdapter::setMultiValue (bool state,
                                                 const QString &value)
{
    if (!valueExists(value))
        return false;

    if (state)
        return insertValue(value);
    else
        return removeValue(value);
}

bool CSMSettings::BooleanAdapter::setSingleValue (const QString &value)
{
    QPair<QString, QBool *> settingPair;

    //search for any values with a true state in the sub model
    foreach (settingPair, mSettings)
    {
        //skip the currently selected value
        if (settingPair.first == value)
            continue;

        if (*(settingPair.second) == QBool(true))
        {
            removeValue(settingPair.first);
            *(settingPair.second) = QBool(false);
        }
    }

    return insertValue(value);
}

Qt::ItemFlags CSMSettings::BooleanAdapter::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QModelIndex CSMSettings::BooleanAdapter::index(int row, int column,
                                               const QModelIndex &parent) const
{
    if ((row >= 0 && row < rowCount()) &&
        ( column >= 0 && column < columnCount()))
    {
        return createIndex (row, column);
    }

    return QModelIndex();
}
