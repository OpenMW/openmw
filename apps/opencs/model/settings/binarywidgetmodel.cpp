#include <QStringList>

#include "settingmodel.hpp"
#include "binarywidgetadapter.hpp"
#include "usersettings.hpp"

CSMSettings::BinaryWidgetAdapter::BinaryWidgetAdapter(SectionFilter *filter,
                                                  const QString &settingName,
                                                  QObject *parent) :
    mValueList(0), mFilter(0), QSortFilterProxyModel(parent)
{
    mFilter = filter;

    setSourceModel(mFilter);
    setFilterKeyColumn(0);
    setFilterRegExp(settingName);
    setDynamicSortFilter(true);

    mSetting = settingName;

    QModelIndex sourceIndex = index(0, 3, QModelIndex());
    mValueList = data(sourceIndex).toStringList();
}

bool CSMSettings::BinaryWidgetAdapter::insertItem(const QString &item)
{
    //if the item isn't found in a pre-defined list (if available), abort
    if (mValueList.size()>0)
    {
        if (!mValueList.contains(item))
            return false;
    }

    //if the item already exists in the model, abort
    if (itemIndex(item).isValid())
        return false;

    mFilter->createSetting(mSetting, item, mValueList);

    return true;
}

bool CSMSettings::BinaryWidgetAdapter::removeItem(const QString &item)
{
    QModelIndex idx = itemIndex(item);

    while (idx.isValid())
    {
        removeRow(idx.row());
        idx = itemIndex(item);
    }

    return true;
}

QModelIndex CSMSettings::BinaryWidgetAdapter::itemIndex(const QString &item)
{
    for (int i = 0; i < rowCount(); i++)
    {
        QModelIndex idx2 = index(i, 2, QModelIndex());
        QModelIndex idx0 = index(i, 0, QModelIndex());
        QModelIndex idx1 = index (i, 1, QModelIndex());

        if (data(idx2).toString() == item)
            return idx2;
    }

    return QModelIndex();
}
