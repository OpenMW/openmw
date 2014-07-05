
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

    return 1;
}

QVariant CSMWorld::ResourceTable::data  (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (index.column()!=0)
        throw std::logic_error ("Invalid column in resource table");

    return QString::fromUtf8 (mResources->getId (index.row()).c_str());
}

QVariant CSMWorld::ResourceTable::headerData (int section, Qt::Orientation orientation,
    int role ) const
{
    if (orientation==Qt::Vertical)
        return QVariant();

    if (role==Qt::DisplayRole)
        return "ID";

    if (role==ColumnBase::Role_Flags)
        return ColumnBase::Flag_Table;

    if (role==ColumnBase::Role_Display)
        return ColumnBase::Display_String;

    return QVariant();
}

bool CSMWorld::ResourceTable::setData ( const QModelIndex &index, const QVariant &value,
    int role)
{
    return false;
}

Qt::ItemFlags CSMWorld::ResourceTable::flags (const QModelIndex & index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;;
}

QModelIndex CSMWorld::ResourceTable::index (int row, int column, const QModelIndex& parent)
    const
{
    if (parent.isValid())
        return QModelIndex();

    if (row<0 || row>=mResources->getSize())
        return QModelIndex();

    if (column!=0)
        return QModelIndex();

    return createIndex (row, column);
}

QModelIndex CSMWorld::ResourceTable::parent (const QModelIndex& index) const
{
    return QModelIndex();
}

QModelIndex CSMWorld::ResourceTable::getModelIndex (const std::string& id, int column) const
{
    return index (mResources->getIndex (id), column);
}

int CSMWorld::ResourceTable::searchColumnIndex (Columns::ColumnId id) const
{
    if (id==Columns::ColumnId_Id)
        return 0;

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