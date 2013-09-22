#include "modelitem.hpp"

ContentSelectorModel::ModelItem::ModelItem(ModelItem *parent)
    : mParentItem(parent)
{
}
/*
ContentSelectorModel::ModelItem::ModelItem(const ModelItem *parent)
   // : mParentItem(parent)
{
}
*/

ContentSelectorModel::ModelItem::~ModelItem()
{
    qDeleteAll(mChildItems);
}


ContentSelectorModel::ModelItem *ContentSelectorModel::ModelItem::parent() const
{
    return mParentItem;
}

bool ContentSelectorModel::ModelItem::hasFormat(const QString &mimetype) const
{
    if (mimetype == "application/omwcontent")
        return true;

    return QMimeData::hasFormat(mimetype);
}
int ContentSelectorModel::ModelItem::row() const
{
    if (mParentItem)
        return 1;
        //return mParentItem->childRow(const_cast<ModelItem*>(this));
        //return mParentItem->mChildItems.indexOf(const_cast<ModelItem*>(this));

    return -1;
}


int ContentSelectorModel::ModelItem::childCount() const
{
    return mChildItems.count();
}

int ContentSelectorModel::ModelItem::childRow(ModelItem *child) const
{
    Q_ASSERT(child);

    return mChildItems.indexOf(child);
}

ContentSelectorModel::ModelItem *ContentSelectorModel::ModelItem::child(int row)
{
    return mChildItems.value(row);
}


void ContentSelectorModel::ModelItem::appendChild(ModelItem *item)
{
    mChildItems.append(item);
}

void ContentSelectorModel::ModelItem::removeChild(int row)
{
    mChildItems.removeAt(row);
}
