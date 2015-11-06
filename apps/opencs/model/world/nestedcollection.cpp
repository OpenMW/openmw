#include "nestedcollection.hpp"

CSMWorld::NestedCollection::NestedCollection()
{}

CSMWorld::NestedCollection::~NestedCollection()
{}

int CSMWorld::NestedCollection::getNestedRowsCount(int row, int column) const
{
    return 0;
}

int CSMWorld::NestedCollection::getNestedColumnsCount(int row, int column) const
{
    return 0;
}

int CSMWorld::NestedCollection::searchNestedColumnIndex(int parentColumn, Columns::ColumnId id)
{
    // Assumed that the parentColumn is always a valid index
    const NestableColumn *parent = getNestableColumn(parentColumn);
    int nestedColumnCount = getNestedColumnsCount(0, parentColumn);
    for (int i = 0; i < nestedColumnCount; ++i)
    {
        if (parent->nestedColumn(i).mColumnId == id)
        {
            return i;
        }
    }
    return -1;
}

int CSMWorld::NestedCollection::findNestedColumnIndex(int parentColumn, Columns::ColumnId id)
{
    int index = searchNestedColumnIndex(parentColumn, id);
    if (index == -1)
    {
        throw std::logic_error("CSMWorld::NestedCollection: No such nested column");
    }
    return index;
}
