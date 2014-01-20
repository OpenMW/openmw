#include <QSortFilterProxyModel>

#include "viewadapter.hpp"
#include "../../view/settings/support.hpp"
#include "definitionmodel.hpp"

#include <QDebug>

CSMSettings::ViewAdapter::ViewAdapter ( DefinitionModel &model,
                                        const QString &pageName,
                                        const QString &settingName,
                                        QObject *parent)

    : mSingleValueMode (true), mModel (model), mPageName (pageName),
      mSettingName (settingName), QAbstractItemModel (parent)
{
    QSortFilterProxyModel *pageFilter =
                                buildFilter(&mModel, Setting_Page, mPageName);

    mSettingFilter = buildFilter (pageFilter, Setting_Name, mSettingName);
}

QVariant CSMSettings::ViewAdapter::sourceData (int row, int column) const
{
    QModelIndex idx = mModel.index (row, column, QModelIndex());

    if (idx.isValid())
        return mModel.data (idx, Qt::DisplayRole);

    return QVariant();
}

bool CSMSettings::ViewAdapter::valueExists (const QString &value,
                                           SettingColumn column) const
{
    for (int i = 0; i < mModel.rowCount(); i++)
    {
        if (sourceData (i, column).toString() == value)
            return true;
    }
    return false;
}

QModelIndex CSMSettings::ViewAdapter::valueSourceIndex
                            (const QString &value, SettingColumn column) const
{
    QModelIndex idx;

    for (int i = 0; i < mSettingFilter->rowCount(); i++)
    {
        idx = mSettingFilter->index (i, column);
        QString sourceValue = mSettingFilter->data(idx).toString();

        if (sourceValue == value)
            return mSettingFilter->mapToSource(idx);
    }

    return QModelIndex();
}

bool CSMSettings::ViewAdapter::insertValue (const QString &value)
{
    QModelIndex idx = valueSourceIndex (value, Setting_Value);

    //abort if the adapter manages a single-vlaued setting and the
    //setting already has a definition in the source model.
    if (idx.isValid() && mSingleValueMode)
        return false;

    return mModel.defineSetting (mSettingName, mPageName, value).isValid();
}

bool CSMSettings::ViewAdapter::removeValue (const QString &value)
{
    QModelIndex idx = valueSourceIndex (value, Setting_Value);

    if (!idx.isValid())
        return false;

    while (idx.isValid())
    {
        if (!mModel.removeRows (idx.row(), 1, QModelIndex()))
            return false;

        idx = valueSourceIndex (value, Setting_Value);
    }

    return true;
}

QSortFilterProxyModel *CSMSettings::ViewAdapter::buildFilter
    (QAbstractItemModel *model, SettingColumn column, const QString &expression)
{
    QSortFilterProxyModel *filter = new QSortFilterProxyModel (this);
    filter->setSourceModel (model);
    filter->setFilterKeyColumn (column);
    filter->setFilterRegExp (expression);
    filter->setObjectName (expression);

    return filter;
}
