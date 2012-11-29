
#include "idtable.hpp"

#include "idcollection.hpp"

CSMWorld::IdTable::IdTable (IdCollectionBase *idCollection) : mIdCollection (idCollection)
{

}

CSMWorld::IdTable::~IdTable()
{

}

int CSMWorld::IdTable::rowCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return mIdCollection->getSize();
}

int CSMWorld::IdTable::columnCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return mIdCollection->getColumns();
}

QVariant CSMWorld::IdTable::data  (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole && role!=Qt::EditRole)
        return QVariant();

    if (role==Qt::EditRole && !mIdCollection->isEditable (index.column()))
            return QVariant();

    return mIdCollection->getData (index.row(), index.column());
}

QVariant CSMWorld::IdTable::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (orientation==Qt::Vertical)
        return QVariant();

    return tr (mIdCollection->getTitle (section).c_str());
}

bool CSMWorld::IdTable::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    if (mIdCollection->isEditable (index.column()) && role==Qt::EditRole)
    {
        mIdCollection->setData (index.row(), index.column(), value);
        emit dataChanged (index, index);
        return true;
    }

    return false;
}

Qt::ItemFlags CSMWorld::IdTable::flags (const QModelIndex & index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (mIdCollection->isEditable (index.column()))
        flags |= Qt::ItemIsEditable;

    return flags;
}