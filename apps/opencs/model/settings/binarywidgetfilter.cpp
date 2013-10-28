#include "binarywidgetfilter.hpp"

BinaryWidgetFilter::BinaryWidgetFilter(QObject *parent) :
    QSortFilterProxyModel(parent)
{}

int BinaryWidgetFilter::rowCount (const QModelIndex &parent) const
{
    return 1;
}

QVariant CSMSettings::BinaryWidgetFilter::data (const QModelIndex &index, int role) const
{
    //return data (index, role);
}

Qt::ItemFlags CSMSettings::BinaryWidgetFilter::flags (const QModelIndex &index) const
{
    //return flags (index);
}

bool CSMSettings::BinaryWidgetFilter::setData (const QModelIndex &index, const QVariant &value, int role)
{
    //return setData (index, value, role);
}
