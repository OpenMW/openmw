#include "setting.hpp"
#include "../../view/settings/support.hpp"

#include <QDebug>
#include <QStandardItem>

CSMSettings::Setting::Setting() : QObject()
{
    buildDefaultSetting();
}

CSMSettings::Setting::Setting(SettingType typ, const QString &name,
                              const QString &page, const QString &defaultValue,
                              QObject *parent) : QObject(parent)
{
    buildDefaultSetting();

    int vType = static_cast <int> (typ);

    bool isMulti = ((vType % 2) == 0);

    if (!isMulti)
        vType--;

    vType = vType / 2;

    QStandardItem *nameItem = mSettingRow.at (Property_Name);

    nameItem->setData (name, Qt::DisplayRole);
    mSettingRow.at (Property_Page)->setData (page, Qt::DisplayRole);
    mSettingRow.at (Property_DefaultValue)->setData
                                                (defaultValue, Qt::DisplayRole);
    mSettingRow.at (Property_ViewType)->setData
                        (QVariant (static_cast <int> (vType)), Qt::DisplayRole);

    mSettingRow.at (Property_IsMultiValue)->setData
                                          (QVariant (isMulti), Qt::DisplayRole);

}

QVariant CSMSettings::Setting::property (SettingProperty prop) const
{
    const QStandardItem *item = mSettingRow.at(prop);

    if (item)
        return item->data(Qt::DisplayRole);

    return QVariant();
}

QStringList CSMSettings::Setting::propertyList
                                        (SettingPropertyList propertyRow) const
{
    const QStandardItem *item = mSettingRow.at(Property_Name);

    if (!item)
        return QStringList();

    return item->child (propertyRow)->data(Qt::UserRole).toStringList();
}

void CSMSettings::Setting::setProperty (int column, QString value)
{
    QStandardItem *item = new QStandardItem();
    item->setData(value, Qt::DisplayRole);
    setProperty(column, item);
}

void CSMSettings::Setting::setProperty (int column, bool value)
{
    setProperty (column, QVariant (value).toString());
}

void CSMSettings::Setting::setProperty (int column, int value)
{
    setProperty (column, QVariant (value).toString());
}

void CSMSettings::Setting::setProperty(int column, QStandardItem *item)
{
    if (column < 0 || column >= mSettingRow.size())
        return;

    mSettingRow.replace (column, item);
}

void CSMSettings::Setting::setPropertyList (SettingPropertyList propList,
                                                        const QStringList &list)
{
    QStandardItem *item = mSettingRow.at (Property_Name);
    QStandardItem *itemList = item->child (propList);

    itemList->setData (list, Qt::UserRole);
    itemList->removeColumn (0);

    QList <QStandardItem *> columnList;

    foreach (const QString &value, list)
        columnList.append (new QStandardItem (value));

    itemList->appendColumn (columnList);
}

void CSMSettings::Setting::buildDefaultSetting()
{
    for (int i = 0; i < sSettingPropertyCount; i++)
        mSettingRow.append (new QStandardItem (sPropertyDefaults[i].value));


    for (int i = 0; i < sSettingPropertyListCount; i++)
    {
        QList <QStandardItem *> childRow;
        QStandardItem *childItem = new QStandardItem (sPropertyListDefaults[i].name);
        childItem->setData (QVariant(mTemp), Qt::UserRole);
        childRow.append (childItem);
        mChildRows.append (childRow);
        mSettingRow.at(Property_Name)->appendRow (childRow);
    }
}
