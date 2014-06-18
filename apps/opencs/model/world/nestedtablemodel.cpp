#include "nestedtablemodel.hpp"

#include "./idtable.hpp"

CSMWorld::NestedTableModel::NestedTableModel(const QModelIndex& parent,
                                             ColumnBase::Display columnId,
                                             CSMWorld::IdTable* parentModel) 
    : mParentColumn(parent.column())
{
    const int parentRow = parent.row();
    mId = std::string(parentModel->index(parentRow, 0).data().toString().toUtf8());
    QAbstractProxyModel::setSourceModel(parentModel);
}

QModelIndex CSMWorld::NestedTableModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    const QModelIndex& testedParent = sourceModel()->parent(sourceIndex);
    const QModelIndex& parent = dynamic_cast<CSMWorld::IdTable*>(sourceModel())->getModelIndex (mId, mParentColumn);
    if (testedParent == parent)
    {
        return createIndex(sourceIndex.row(), sourceIndex.column());
    }
    else 
    {
        return QModelIndex();
    }

}

QModelIndex CSMWorld::NestedTableModel::mapToSource(const QModelIndex& proxyIndex) const
{
    const QModelIndex& parent = dynamic_cast<CSMWorld::IdTable*>(sourceModel())->getModelIndex (mId, mParentColumn);
    return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), parent);
}
