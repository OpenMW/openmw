#include "nestedtablemodel.hpp"

#include <cassert>
#include "./idtable.hpp"

CSMWorld::NestedTableModel::NestedTableModel(const QModelIndex& parent,
                                             ColumnBase::Display columnId,
                                             CSMWorld::IdTable* parentModel)
    : mParentColumn(parent.column()),
      mMainModel(parentModel)
{
    const int parentRow = parent.row();

    mId = std::string(parentModel->index(parentRow, 0).data().toString().toUtf8());

    QAbstractProxyModel::setSourceModel(parentModel);
}

QModelIndex CSMWorld::NestedTableModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    const QModelIndex& testedParent = mMainModel->parent(sourceIndex);
    const QModelIndex& parent = mMainModel->getModelIndex (mId, mParentColumn);
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
    const QModelIndex& parent = mMainModel->getModelIndex (mId, mParentColumn);
    return mMainModel->index(proxyIndex.row(), proxyIndex.column(), parent);
}

int CSMWorld::NestedTableModel::rowCount(const QModelIndex& index) const
{
    assert (!index.isValid());

    return mMainModel->rowCount(mMainModel->getModelIndex(mId, mParentColumn));
}

int CSMWorld::NestedTableModel::columnCount(const QModelIndex& parent) const
{
    assert (!parent.isValid());

    return mMainModel->columnCount(mMainModel->getModelIndex(mId, mParentColumn));
}

QModelIndex CSMWorld::NestedTableModel::index(int row, int column, const QModelIndex& parent) const
{
    assert (!parent.isValid());

    int rows = mMainModel->rowCount(parent);
    int columns = mMainModel->columnCount(parent);

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

QVariant CSMWorld::NestedTableModel::headerData(int section,
                                                Qt::Orientation orientation,
                                                int role) const
{
    return mMainModel->nestedHeaderData(mParentColumn, section, orientation, role);
}
