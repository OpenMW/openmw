#include "binarywidgetmodel.hpp"
#include <QStringList>

#include <QDebug>

CSMSettings::BinaryWidgetModel::BinaryWidgetModel(QStringList &values, QObject *parent) :
    mValueList(values), QObject (parent)
{}

bool CSMSettings::BinaryWidgetModel::insertItem(const QString &item)
{
    qDebug() << "BinaryWidgetModel::insertItem()::item selected for insertion: " << item;
    if (mValueList.contains(item))
        return false;

    mValueList.append(item);

    qDebug() << "BinaryWidgetModel::insertItem()::inserting item " << item;
    return true;
}

bool CSMSettings::BinaryWidgetModel::removeItem(const QString &item)
{
    qDebug() << "BinaryWidgetModel::removeItem()::item selected for removal: " << item;
    if (!mValueList.contains(item))
        return false;

    qDebug() << "BinaryWidgetModel::removeItem()::removing item: " << item;

    return (mValueList.removeAll(item) > 0);
}
