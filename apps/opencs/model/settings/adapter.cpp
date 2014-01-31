#include <QSortFilterProxyModel>

#include "adapter.hpp"
#include "../../view/settings/support.hpp"
#include "definitionmodel.hpp"

#include <QDebug>

CSMSettings::Adapter::Adapter ( DefinitionModel &model,
                                        const QString &pageName,
                                        const QString &settingName,
                                        bool isMultiValue,
                                        QObject *parent)

    : mIsMultiValue (isMultiValue), mModel (model),
      mPageName (pageName), mSettingName (settingName),
      QAbstractItemModel (parent)
{
    QSortFilterProxyModel *pageFilter =
                                buildFilter(&mModel, Setting_Page, mPageName);

    mSettingFilter = buildFilter (pageFilter, Setting_Name, mSettingName);

    connect (mSettingFilter,
             SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
             this,
             SLOT(slotDataChanged(const QModelIndex &, const QModelIndex &)));

    connect (mSettingFilter, SIGNAL(layoutChanged()),
             this, SLOT(slotLayoutChanged()));
}

int CSMSettings::Adapter::rowCount (const QModelIndex &parent) const
{
    return mSettingFilter->rowCount();
}

int CSMSettings::Adapter::columnCount (const QModelIndex &parent) const
{
    return mSettingFilter->columnCount();
}

bool CSMSettings::Adapter::valueExists (const QString &value)
{
    bool success = false;

    for (int i = 0; i < rowCount(); i++)
    {
        success = (valueSourceIndex (value, Setting_Value).isValid());

        if (success)
            break;
    }

    return success;
}

bool CSMSettings::Adapter::validIndex (QModelIndex idx) const
{
    if (!idx.isValid())
        return false;

    if (idx.row() < 0 || idx.row() >= rowCount())
        return false;

    return true;
}

Qt::ItemFlags CSMSettings::Adapter::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QModelIndex CSMSettings::Adapter::valueSourceIndex
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

bool CSMSettings::Adapter::insertValue (const QString &value)
{
    QModelIndex idx = valueSourceIndex (value, Setting_Value);

    //abort if the adapter manages a single-vlaued setting and the
    //setting already has a definition in the source model.
    if (idx.isValid() && !mIsMultiValue)
        return false;

    return mModel.defineSetting (mSettingName, mPageName, value).isValid();
}

bool CSMSettings::Adapter::removeValue (const QString &value)
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

QSortFilterProxyModel *CSMSettings::Adapter::buildFilter
    (QAbstractItemModel *model, SettingColumn column, const QString &expression)
{

    QSortFilterProxyModel *filter = new QSortFilterProxyModel (this);
    filter->setSourceModel (model);
    filter->setFilterKeyColumn (column);
    filter->setFilterRegExp (expression);
    filter->setObjectName (expression);

    return filter;
}

QModelIndex CSMSettings::Adapter::index(int row, int column,
                                               const QModelIndex &parent) const
{
    if ((row >= 0 && row < rowCount()) &&
        ( column >= 0 && column < columnCount()))
    {
        return createIndex (row, column);
    }

    return QModelIndex();
}
