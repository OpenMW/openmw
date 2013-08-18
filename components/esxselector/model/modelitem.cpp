#include "modelitem.hpp"

EsxModel::ModelItem::ModelItem(ModelItem *parent)
    : mParentItem(parent)
    , QObject(parent)
{
}

EsxModel::ModelItem::~ModelItem()
{
    qDeleteAll(mChildItems);
}


EsxModel::ModelItem *EsxModel::ModelItem::parent()
{
    return mParentItem;
}

int EsxModel::ModelItem::row() const
{
    if (mParentItem)
        return 1;
        //return mParentItem->childRow(const_cast<ModelItem*>(this));
        //return mParentItem->mChildItems.indexOf(const_cast<ModelItem*>(this));

    return -1;
}


int EsxModel::ModelItem::childCount() const
{
    return mChildItems.count();
}

int EsxModel::ModelItem::childRow(ModelItem *child) const
{
    Q_ASSERT(child);

    return mChildItems.indexOf(child);
}

EsxModel::ModelItem *EsxModel::ModelItem::child(int row)
{
    return mChildItems.value(row);
}


void EsxModel::ModelItem::appendChild(ModelItem *item)
{
    mChildItems.append(item);
}

void EsxModel::ModelItem::removeChild(int row)
{
    mChildItems.removeAt(row);
}
