#include "binarywidgetfilter.hpp"
#include <QStringList>

#include <QDebug>

CSMSettings::BinaryWidgetFilter::BinaryWidgetFilter(const QString &matchValue,
                                                    QObject *parent) :
    QSortFilterProxyModel(parent), mMatchValue(matchValue)
{}

int CSMSettings::BinaryWidgetFilter::rowCount (const QModelIndex &parent) const
{
    return 1;
}

QVariant CSMSettings::BinaryWidgetFilter::data (const QModelIndex &index,
                                                                int role) const
{
    QModelIndex sourceIndex = mapToSource(index);
    QStringList values = sourceModel()->data(sourceIndex, role).toStringList();
    qDebug() << "binFilter::data()::value: " << values << "; match: " << mMatchValue << "; index: " << index;
    if (values.contains(mMatchValue))
        return true;

    return false;
}

Qt::ItemFlags CSMSettings::BinaryWidgetFilter::flags
                                                (const QModelIndex &index) const
{
    QModelIndex sourceIndex = mapToSource(index);
    return sourceModel()->flags (sourceIndex);
}

bool CSMSettings::BinaryWidgetFilter::setData (const QModelIndex &index,
                                               const QVariant &value, int role)
{
    qDebug() << "binFilter::setData()::" << mMatchValue << " = " << value.toBool();

    if (!index.isValid())
        return false;

    QModelIndex sourceIndex = mapToSource(index);

    if (!sourceIndex.isValid())
        return false;

    if (value.toBool())
    {
        qDebug() << "binFilter::setData()::adding value: " << mMatchValue;
        return sourceModel()->setData (sourceIndex, mMatchValue, role);
    }
    else
    {
        if (value.toString() == sourceModel()->data(sourceIndex, role).toStringList().at(sourceIndex.row()))
        {
            qDebug() << "binFilter::setData()::removing value: " << mMatchValue << " from row " << sourceIndex.row();
            return sourceModel()->removeRow(sourceIndex.row(), QModelIndex());
        }
    }
    return false;
}
