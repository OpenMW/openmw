#include "nestedtableproxymodel.hpp"

#include <cassert>
#include "idtree.hpp"

CSMWorld::NestedTableProxyModel::NestedTableProxyModel(const QModelIndex& parent,
                                             ColumnBase::Display columnId,
                                             CSMWorld::IdTree* parentModel)
    : mParentColumn(parent.column()),
      mMainModel(parentModel)
{
    const int parentRow = parent.row();

    mId = std::string(parentModel->index(parentRow, 0).data().toString().toUtf8());

    QAbstractProxyModel::setSourceModel(parentModel);

    connect(mMainModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(forwardRowsAboutToInserted(const QModelIndex &, int, int)));

    connect(mMainModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(forwardRowsInserted(const QModelIndex &, int, int)));

    connect(mMainModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(forwardRowsAboutToRemoved(const QModelIndex &, int, int)));

    connect(mMainModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this, SLOT(forwardRowsRemoved(const QModelIndex &, int, int)));

    connect(mMainModel, SIGNAL(resetStart(const QString&)),
            this, SLOT(forwardResetStart(const QString&)));

    connect(mMainModel, SIGNAL(resetEnd(const QString&)),
            this, SLOT(forwardResetEnd(const QString&)));

    connect(mMainModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(forwardDataChanged(const QModelIndex &, const QModelIndex &)));
}

QModelIndex CSMWorld::NestedTableProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    const QModelIndex& testedParent = mMainModel->parent(sourceIndex);
    const QModelIndex& parent = mMainModel->getNestedModelIndex (mId, mParentColumn);
    if (testedParent == parent)
    {
        return createIndex(sourceIndex.row(), sourceIndex.column());
    }
    else
    {
        return QModelIndex();
    }
}

QModelIndex CSMWorld::NestedTableProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    const QModelIndex& parent = mMainModel->getNestedModelIndex (mId, mParentColumn);
    return mMainModel->index(proxyIndex.row(), proxyIndex.column(), parent);
}

int CSMWorld::NestedTableProxyModel::rowCount(const QModelIndex& index) const
{
    assert (!index.isValid());

    return mMainModel->rowCount(mMainModel->getModelIndex(mId, mParentColumn));
}

int CSMWorld::NestedTableProxyModel::columnCount(const QModelIndex& parent) const
{
    assert (!parent.isValid());

    return mMainModel->columnCount(mMainModel->getModelIndex(mId, mParentColumn));
}

QModelIndex CSMWorld::NestedTableProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    assert (!parent.isValid());

    int numRows = rowCount(parent);
    int numColumns = columnCount(parent);

    if (row < 0 || row >= numRows || column < 0 || column >= numColumns)
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex CSMWorld::NestedTableProxyModel::parent(const QModelIndex& index) const
{
    return QModelIndex();
}

QVariant CSMWorld::NestedTableProxyModel::headerData(int section,
                                                Qt::Orientation orientation,
                                                int role) const
{
    return mMainModel->nestedHeaderData(mParentColumn, section, orientation, role);
}

QVariant CSMWorld::NestedTableProxyModel::data(const QModelIndex& index, int role) const
{
    return mMainModel->data(mapToSource(index), role);
}

// NOTE: Due to mapToSouce(index) the dataChanged() signal resulting from setData() will have the
// source model's index values.  The indicies need to be converted to the proxy space values.
// See forwardDataChanged()
bool CSMWorld::NestedTableProxyModel::setData (const QModelIndex & index, const QVariant & value, int role)
{
    return mMainModel->setData(mapToSource(index), value, role);
}

Qt::ItemFlags CSMWorld::NestedTableProxyModel::flags(const QModelIndex& index) const
{
    return mMainModel->flags(mapToSource(index));
}

std::string CSMWorld::NestedTableProxyModel::getParentId() const
{
    return mId;
}

int CSMWorld::NestedTableProxyModel::getParentColumn() const
{
    return mParentColumn;
}

CSMWorld::IdTree* CSMWorld::NestedTableProxyModel::model() const
{
    return mMainModel;
}

void CSMWorld::NestedTableProxyModel::forwardRowsAboutToInserted(const QModelIndex& parent,
        int first, int last)
{
    if (indexIsParent(parent))
    {
        beginInsertRows(QModelIndex(), first, last);
    }
}

void CSMWorld::NestedTableProxyModel::forwardRowsInserted(const QModelIndex& parent, int first, int last)
{
    if (indexIsParent(parent))
    {
        endInsertRows();
    }
}

bool CSMWorld::NestedTableProxyModel::indexIsParent(const QModelIndex& index)
{
    return (index.isValid() &&
            index.column() == mParentColumn &&
            mMainModel->data(mMainModel->index(index.row(), 0)).toString().toUtf8().constData() == mId);
}

void CSMWorld::NestedTableProxyModel::forwardRowsAboutToRemoved(const QModelIndex& parent,
        int first, int last)
{
    if (indexIsParent(parent))
    {
        beginRemoveRows(QModelIndex(), first, last);
    }
}

void CSMWorld::NestedTableProxyModel::forwardRowsRemoved(const QModelIndex& parent, int first, int last)
{
    if (indexIsParent(parent))
    {
        endRemoveRows();
    }
}

void CSMWorld::NestedTableProxyModel::forwardResetStart(const QString& id)
{
    if (id.toUtf8() == mId.c_str())
        beginResetModel();
}

void CSMWorld::NestedTableProxyModel::forwardResetEnd(const QString& id)
{
    if (id.toUtf8() == mId.c_str())
        endResetModel();
}

void CSMWorld::NestedTableProxyModel::forwardDataChanged (const QModelIndex& topLeft,
        const QModelIndex& bottomRight)
{
    const QModelIndex& parent = mMainModel->getNestedModelIndex (mId, mParentColumn);

    if (topLeft.column() <= parent.column() && bottomRight.column() >= parent.column())
    {
        emit dataChanged(index(0,0),
                index(mMainModel->rowCount(parent)-1, mMainModel->columnCount(parent)-1));
    }
    else if (topLeft.parent() == parent && bottomRight.parent() == parent)
    {
        emit dataChanged(index(topLeft.row(), topLeft.column()), index(bottomRight.row(), bottomRight.column()));
    }
}
