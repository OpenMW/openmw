#include "idtablebase.hpp"

#include "universalid.hpp"
#include "variantconversion.hpp"

CSMWorld::IdTableBase::IdTableBase(unsigned int features)
    : mFeatures(features)
{
}

QVariant CSMWorld::IdTableBase::getData(int row, int column, int role) const
{
    return data(index(row, column), role);
}

CSMWorld::UniversalId CSMWorld::IdTableBase::getUniversalId(int row) const
{
    int idColumn = findColumnIndex(Columns::ColumnId_Id);
    int typeColumn = findColumnIndex(Columns::ColumnId_RecordType);

    const auto type = static_cast<UniversalId::Type>(getData(row, typeColumn).toInt());
    const auto id = getString(getData(row, idColumn));
    return UniversalId(type, id);
}

CSMWorld::ColumnBase::Display CSMWorld::IdTableBase::getColumnDisplay(int column) const
{
    return static_cast<ColumnBase::Display>(headerData(column, Qt::Horizontal, ColumnBase::Role_Display).toInt());
}

unsigned int CSMWorld::IdTableBase::getFeatures() const
{
    return mFeatures;
}
