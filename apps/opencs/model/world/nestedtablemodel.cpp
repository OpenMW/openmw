#include "nestedtablemodel.hpp"

#include <cassert>
#include "./idtable.hpp"
#include <QDebug>

CSMWorld::NestedTableModel::NestedTableModel(const QModelIndex& parent,
                                             ColumnBase::Display columnId,
                                             CSMWorld::IdTable* parentModel)
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


bool CSMWorld::NestedTableModel::setData ( const QModelIndex & index, const QVariant & value, int role)
{
    return mMainModel->setData(mapToSource(index), value, role);
}

Qt::ItemFlags CSMWorld::NestedTableModel::flags(const QModelIndex& index) const
{
    return mMainModel->flags(mMainModel->index(0, mParentColumn));
}

std::string CSMWorld::NestedTableModel::getParentId() const
{
    return mId;
}

int CSMWorld::NestedTableModel::getParentColumn() const
{
    return mParentColumn;
}

CSMWorld::IdTable* CSMWorld::NestedTableModel::model() const
{
    return mMainModel;
}

void CSMWorld::NestedTableModel::forwardRowsAboutToInserted(const QModelIndex& parent, int first, int last)
{
    if (indexIsParent(parent))
    {
        qDebug()<<"Adding new rows "<< first<<":"<<last;
        beginInsertRows(QModelIndex(), first, last);
    }
}

void CSMWorld::NestedTableModel::forwardRowsInserted(const QModelIndex& parent, int first, int last)
{
    if (indexIsParent(parent))
    {
        qDebug()<<"rows added"<< first<<":"<<last;
        endInsertRows();
    }
}

bool CSMWorld::NestedTableModel::indexIsParent(const QModelIndex& index)
{
    qDebug()<<"Testing for parenty";
    qDebug()<<index.isValid();
    qDebug()<<(index.column() == mParentColumn);
    return (index.isValid() &&
            index.column() == mParentColumn &&
            mMainModel->data(mMainModel->index(index.row(), 0)).toString().toUtf8().constData() == mId);
}

void CSMWorld::NestedTableModel::forwardRowsAboutToRemoved(const QModelIndex& parent, int first, int last)
{
    if (indexIsParent(parent))
    {
        beginRemoveRows(QModelIndex(), first, last);
    }
}

void CSMWorld::NestedTableModel::forwardRowsRemoved(const QModelIndex& parent, int first, int last)
{
    if (indexIsParent(parent))
    {
        endRemoveRows();
    }
}
