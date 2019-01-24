#include "resourcetable.hpp"

#include <stdexcept>

#include "resources.hpp"
#include "columnbase.hpp"
#include "universalid.hpp"

CSMWorld::ResourceTable::ResourceTable (const Resources *resources, unsigned int features)
: IdTableBase (features | Feature_Constant), mResources (resources)
{}

CSMWorld::ResourceTable::~ResourceTable() {}

int CSMWorld::ResourceTable::rowCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return mResources->getSize();
}

int CSMWorld::ResourceTable::columnCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return 2; // ID, type
}

QVariant CSMWorld::ResourceTable::data  (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (index.column()==0)
        return QString::fromUtf8 (mResources->getId (index.row()).c_str());

    if (index.column()==1)
        return static_cast<int> (mResources->getType());

    throw std::logic_error ("Invalid column in resource table");
}

QVariant CSMWorld::ResourceTable::headerData (int section, Qt::Orientation orientation,
    int role ) const
{
    if (orientation==Qt::Vertical)
        return QVariant();

    if (role==ColumnBase::Role_Flags)
        return section==0 ? ColumnBase::Flag_Table : 0;

    switch (section)
    {
        case 0:

            if (role==Qt::DisplayRole)
                return Columns::getName (Columns::ColumnId_Id).c_str();

            if (role==ColumnBase::Role_Display)
                return ColumnBase::Display_Id;

            break;

        case 1:

            if (role==Qt::DisplayRole)
                return Columns::getName (Columns::ColumnId_RecordType).c_str();

            if (role==ColumnBase::Role_Display)
                return ColumnBase::Display_Integer;

            break;
    }

    return QVariant();
}

bool CSMWorld::ResourceTable::setData ( const QModelIndex &index, const QVariant &value,
    int role)
{
    return false;
}

Qt::ItemFlags CSMWorld::ResourceTable::flags (const QModelIndex & index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QModelIndex CSMWorld::ResourceTable::index (int row, int column, const QModelIndex& parent)
    const
{
    if (parent.isValid())
        return QModelIndex();

    if (row<0 || row>=mResources->getSize())
        return QModelIndex();

    if (column<0 || column>1)
        return QModelIndex();

    return createIndex (row, column);
}

QModelIndex CSMWorld::ResourceTable::parent (const QModelIndex& index) const
{
    return QModelIndex();
}

QModelIndex CSMWorld::ResourceTable::getModelIndex (const std::string& id, int column) const
{
    int row = mResources->searchId(id);
    if (row != -1)
        return index (row, column);

    return QModelIndex();
}

int CSMWorld::ResourceTable::searchColumnIndex (Columns::ColumnId id) const
{
    if (id==Columns::ColumnId_Id)
        return 0;

    if (id==Columns::ColumnId_RecordType)
        return 1;

    return -1;
}

int CSMWorld::ResourceTable::findColumnIndex (Columns::ColumnId id) const
{
    int index = searchColumnIndex (id);

    if (index==-1)
        throw std::logic_error ("invalid column index");

    return index;
}

std::pair<CSMWorld::UniversalId, std::string> CSMWorld::ResourceTable::view (int row) const
{
    return std::make_pair (UniversalId::Type_None, "");
}

bool CSMWorld::ResourceTable::isDeleted (const std::string& id) const
{
    return false;
}

int CSMWorld::ResourceTable::getColumnId (int column) const
{
    switch (column)
    {
        case 0: return Columns::ColumnId_Id;
        case 1: return Columns::ColumnId_RecordType;
    }

    return -1;
}

void CSMWorld::ResourceTable::beginReset()
{
    beginResetModel();
}

void CSMWorld::ResourceTable::endReset()
{
    endResetModel();
}
