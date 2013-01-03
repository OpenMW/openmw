
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

    if (role==Qt::EditRole && !mIdCollection->getColumn (index.column()).isEditable())
        return QVariant();

    return mIdCollection->getData (index.row(), index.column());
}

QVariant CSMWorld::IdTable::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation==Qt::Vertical)
        return QVariant();

    if (role==Qt::DisplayRole)
        return tr (mIdCollection->getColumn (section).mTitle.c_str());

    if (role==ColumnBase::Role_Flags)
        return mIdCollection->getColumn (section).mFlags;

    return QVariant();
}

bool CSMWorld::IdTable::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    if (mIdCollection->getColumn (index.column()).isEditable() && role==Qt::EditRole)
    {
        mIdCollection->setData (index.row(), index.column(), value);

        emit dataChanged (CSMWorld::IdTable::index (index.row(), 0),
            CSMWorld::IdTable::index (index.row(), mIdCollection->getColumns()-1));

        return true;
    }

    return false;
}

Qt::ItemFlags CSMWorld::IdTable::flags (const QModelIndex & index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (mIdCollection->getColumn (index.column()).isUserEditable())
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool CSMWorld::IdTable::removeRows (int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    beginRemoveRows (parent, row, row+count-1);

    mIdCollection->removeRows (row, count);

    endRemoveRows();

    return true;
}

void CSMWorld::IdTable::addRecord (const std::string& id)
{
    int index = mIdCollection->getSize();

    beginInsertRows (QModelIndex(), index, index);

    mIdCollection->appendBlankRecord (id);

    endInsertRows();
}

QModelIndex CSMWorld::IdTable::getModelIndex (const std::string& id, int column) const
{
    return index (mIdCollection->getIndex (id), column);
}

void CSMWorld::IdTable::setRecord (const RecordBase& record)
{
    int index = mIdCollection->searchId (mIdCollection->getId (record));

    if (index==-1)
    {
        int index = mIdCollection->getSize();

        beginInsertRows (QModelIndex(), index, index);

        mIdCollection->appendRecord (record);

        endInsertRows();
    }
    else
    {
        mIdCollection->replace (index, record);
        emit dataChanged (CSMWorld::IdTable::index (index, 0),
            CSMWorld::IdTable::index (index, mIdCollection->getColumns()-1));
    }
}

const CSMWorld::RecordBase& CSMWorld::IdTable::getRecord (const std::string& id) const
{
    return mIdCollection->getRecord (id);
}