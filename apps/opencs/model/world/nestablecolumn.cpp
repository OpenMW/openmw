#include "nestablecolumn.hpp"

#include <stdexcept>

void CSMWorld::NestableColumn::addColumn(CSMWorld::NestableColumn *column)
{
    mNestedColumns.push_back(column);
    mHasChildren = true;
}

const CSMWorld::ColumnBase& CSMWorld::NestableColumn::nestedColumn(int subColumn) const
{
    if (!mHasChildren)
        throw std::logic_error("Tried to access nested column of the non-nest column");

    return *mNestedColumns.at(subColumn);
}

int CSMWorld::NestableColumn::nestedColumnCount() const
{
    if (!mHasChildren)
        throw std::logic_error("Tried to access number of the subcolumns in the non-nest column");

    return mNestedColumns.size();
}

CSMWorld::NestableColumn::NestableColumn(int columnId, CSMWorld::ColumnBase::Display displayType,
    int flag, const CSMWorld::NestableColumn* parent)
    : mParent(parent), mHasChildren(false), CSMWorld::ColumnBase(columnId, displayType, flag)
{
}

CSMWorld::NestableColumn::~NestableColumn()
{
    for (unsigned int i = 0; i < mNestedColumns.size(); ++i)
    {
        delete mNestedColumns[i];
    }
}

bool CSMWorld::NestableColumn::hasChildren() const
{
    return mHasChildren;
}
