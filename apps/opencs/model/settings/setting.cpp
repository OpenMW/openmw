#include "setting.hpp"

CSMSettings::Setting::Setting(QObject *parent) : QObject(parent)
{
    //create a list with the required number of entries for a setting
    for (int i = 0; i < sSettingPropertyCount; i++)
        mSettingRow.append(0);
}

QVariant CSMSettings::Setting::item (SettingProperty prop) const
{
    SettingItem *item = mSettingRow.at(prop);

    if (item)
        return item->data();

    return QVariant();
}

QStringList &CSMSettings::Setting::itemList
                                        (SettingPropertyList propertyList) const
{
    SettingItem *item = mSettingRow.at(0);

    if (item)
        return item->childValues(propertyList);

    return QStringList();
}

bool CSMSettings::Setting::setRowItem(int column, SettingItem *item)
{
    if (column < 0 || column >= mSettingRow.size())
        return false;

    mSettingRow.replace(column, item);
}
