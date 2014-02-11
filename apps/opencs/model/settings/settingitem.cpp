#include <QStringList>

#include "settingitem.hpp"

CSMSettings::SettingItem::SettingItem (const QString &value) :
    QStandardItem (value)
{}

void CSMSettings::SettingItem::setChildValues (const QStringList &values)
{
    foreach (const QString &value, values)
        appendRow(new SettingItem(value));
}

const QStringList &CSMSettings::SettingItem::childValues
                                        (SettingPropertyList propertyList) const
{
    SettingItem *childItem = static_cast<SettingItem *>(child(propertyList));

    if (childItem)
        return childItem->values();

    return QStringList();
}

QStringList CSMSettings::SettingItem::childValues
                                        (SettingPropertyList propertyList)
{
    SettingItem *childItem = static_cast<SettingItem *>(child(propertyList));

    if (childItem)
        return childItem->values();

    return QStringList();
}

const QStringList &CSMSettings::SettingItem::values() const
{
    return mValues;
}

QStringList CSMSettings::SettingItem::values()
{
   return mValues;
}
