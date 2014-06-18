#include "nestedtablemodel.hpp"

#include <cassert>
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

int CSMWorld::NestedTableModel::rowCount(const QModelIndex& index) const
{
    assert (!index.isValid());

    CSMWorld::IdTable* table = dynamic_cast<CSMWorld::IdTable*>(sourceModel());

    return table->rowCount(table->getModelIndex(mId, mParentColumn));
}

int CSMWorld::NestedTableModel::columnCount(const QModelIndex& parent) const
{
    assert (!parent.isValid());

    CSMWorld::IdTable* table = dynamic_cast<CSMWorld::IdTable*>(sourceModel());

    return table->columnCount(table->getModelIndex(mId, mParentColumn));
}

QModelIndex CSMWorld::NestedTableModel::index(int row, int column, const QModelIndex& parent) const
{
    assert (!parent.isValid());

    CSMWorld::IdTable* table = dynamic_cast<CSMWorld::IdTable*>(sourceModel());

    unsigned rows = table->rowCount(parent);
    usigned columns = table->columnCount(parent);

    if (row < 0 ||
        row >= rows ||
        column < 0 ||
        column >= columns)
    {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex CSMWorld::NestedTableModel::parent(const QModelIndex& index) const
{
    return QModelIndex();
}
