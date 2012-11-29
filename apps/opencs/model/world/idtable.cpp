
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

    return 1+mIdCollection->getColumns();
}

QVariant CSMWorld::IdTable::data  (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole && role!=Qt::EditRole)
        return QVariant();

    if (role==Qt::EditRole)
    {
        if (index.column()==0)
            return QVariant();

        if (!mIdCollection->isEditable (index.column()-1))
            return QVariant();
    }

    if (index.column()==0)
        return QVariant (tr (mIdCollection->getId (index.row()).c_str()));

    return mIdCollection->getData (index.row(), index.column()-1);
}

QVariant CSMWorld::IdTable::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (orientation==Qt::Vertical)
        return QVariant();

    if (section==0)
        return QVariant (tr ("ID"));

    return tr (mIdCollection->getTitle (section-1).c_str());
}

bool CSMWorld::IdTable::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column()>0 && role==Qt::EditRole)
    {
        mIdCollection->setData (index.row(), index.column()-1, value);
        emit dataChanged (index, index);
        return true;
    }

    return false;
}

Qt::ItemFlags CSMWorld::IdTable::flags (const QModelIndex & index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (index.column()>0)
        flags |= Qt::ItemIsEditable;

    return flags;
}