#include <QStandardItemModel>
#include <QStandardItem>

#include "settingfiltermodel.hpp"

CSMSettings::SettingFilterModel::SettingFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{}

void CSMSettings::SettingFilterModel::addFilterExpression
            (const QString &filterName, int column, const QString &expression)
{
    mFilters[filterName].append(ExpPair(column, expression));
}

void CSMSettings::SettingFilterModel::addModelColumn (const QStringList &list)
{
    QList <QStandardItem *>itemList;

    foreach (const QString value, list)
        itemList.append (new QStandardItem (value));

    mModel.appendColumn (itemList);
}

void CSMSettings::SettingFilterModel::setFilterExpression
            (const QString &filterName, int column, const QString & expression)
{
    if (!mFilters.keys().contains(filterName))
        return;

    Filter filter = mFilters[filterName];
/*
    foreach (const ExpPair &pair, list)
    {
        if (pair.first == column)
        {
            pair.second = expression;
            break;
        }
    }*/
}

void CSMSettings::SettingFilterModel::setCurrentFilter
            (const QString &filterName)
{
    if (!mFilters.keys().contains (filterName))
        return;

    mCurrentFilter = mFilters[filterName];
}

bool CSMSettings::SettingFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if(mFilters.isEmpty())
        return true;

    bool ret = false;

    foreach (ExpPair pair, mCurrentFilter)
    {
        QModelIndex index =
                    sourceModel()->index(sourceRow, pair.first, sourceParent);

        ret = (index.data().toString() == pair.second);

        if(!ret)
            return ret;
    }
    return ret;
}

