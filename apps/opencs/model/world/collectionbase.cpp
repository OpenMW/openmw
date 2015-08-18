#include "collectionbase.hpp"

#include <stdexcept>

#include "columnbase.hpp"

CSMWorld::CollectionBase::CollectionBase() {}

CSMWorld::CollectionBase::~CollectionBase() {}

int CSMWorld::CollectionBase::searchColumnIndex (Columns::ColumnId id) const
{
    int columns = getColumns();

    for (int i=0; i<columns; ++i)
        if (getColumn (i).mColumnId==id)
            return i;

    return -1;
}

int CSMWorld::CollectionBase::findColumnIndex (Columns::ColumnId id) const
{
    int index = searchColumnIndex (id);

    if (index==-1)
        throw std::logic_error ("invalid column index");

    return index;
}
