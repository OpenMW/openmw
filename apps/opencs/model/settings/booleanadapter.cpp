#include <QStringList>

#include "definitionmodel.hpp"
#include "booleanadapter.hpp"
#include "setting.hpp"

#include <QDebug>

CSMSettings::BooleanAdapter::BooleanAdapter (DefinitionModel &model,
                                            const CSMSettings::Setting *setting,
                                            QObject *parent)

    : Adapter (model, setting->pageName, setting->settingName,
               setting->isMultiValue, parent)
{

    //create a list of QString pairs which represent the setting values and
    //whether or not they are set (true / false)
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
}

void CSMSettings::BooleanAdapter::slotLayoutChanged()
{
    //iterate settings, updating their state to reflect
    //the values present / missing in the model
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

    return Adapter::insertValue (value);
}

bool CSMSettings::BooleanAdapter::removeValue (const QString &value)
{
    //if the item isn't found in the local model, abort
    if (!valueExists (value))
        return false;

     return Adapter::removeValue (value);
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


bool CSMSettings::BooleanAdapter::setData(const QModelIndex &index,
                                          const QVariant &value, int role)
{
    if (!validIndex(index))
        return false;

    QString item = mSettings.at(index.row()).first;
    bool success = false;

    if (!isMultiValue())
        success = setSingleValue(item);
    else
        success = setMultiValue(value.toBool(), item);

    if (!success)
        return false;

    *(mSettings.at(index.row()).second) = QBool(value.toBool());

    emit dataChanged(index, index);

    return true;
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
