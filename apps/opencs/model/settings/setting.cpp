#include "setting.hpp"

#include <QDebug>
#include <QStandardItem>

CSMSettings::Setting::Setting(QObject *parent) : QObject(parent)
{
    //create a list with the required number of entries for a setting
    for (int i = 0; i < sSettingPropertyCount; i++)
        mSettingRow.append(new QStandardItem());
}

const QVariant &CSMSettings::Setting::property (SettingProperty prop) const
{
    const QStandardItem *item = mSettingRow.at(prop);

    if (item)
        return item->data(Qt::DisplayRole);

    return QVariant();
}

const QStringList &CSMSettings::Setting::propertyList
                                        (SettingPropertyList propertyRow) const
{
    const QStandardItem *item = mSettingRow.at(0);

    if (item)
        return item->data(Qt::UserRole).toStringList();

    return QStringList();
}

void CSMSettings::Setting::setProperty (int column, QString value)
{
    QStandardItem *item = new QStandardItem();
    item->setData(value, Qt::DisplayRole);
    setProperty(column, item);
    qDebug() << "set property " << column << " = " << value;
    qDebug() << "stored value = " << item->data(Qt::DisplayRole);
}

bool CSMSettings::Setting::setProperty(int column, QStandardItem *item)
{
    if (column < 0 || column >= mSettingRow.size())
        return false;

    mSettingRow.replace(column, item);
}

void CSMSettings::Setting::setPropertyList (SettingPropertyList propList,
                                                        const QStringList &list)
{
    QStandardItem *item = mSettingRow.at(0);

    if (propList > -1 && propList < item->rowCount())
        item->child(propList)->setData(list);
}
