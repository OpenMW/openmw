#include "columnbase.hpp"

#include "columns.hpp"

CSMWorld::ColumnBase::ColumnBase (int columnId, Display displayType, int flags, bool canNest)
    : mColumnId (columnId), mDisplayType (displayType), mFlags (flags), mCanNest(canNest)
{}

CSMWorld::ColumnBase::~ColumnBase() {}

bool CSMWorld::ColumnBase::isUserEditable() const
{
    return isEditable();
}

std::string CSMWorld::ColumnBase::getTitle() const
{
    return Columns::getName (static_cast<Columns::ColumnId> (mColumnId));
}

int  CSMWorld::ColumnBase::getId() const
{
    return mColumnId;
}

bool CSMWorld::NestColumn::canHaveNestedColumns() const
{
    return mCanNest;
}

void CSMWorld::NestColumn::addNestedColumn(int columnId, Display displayType)
{
    if (!mCanNest)
        throw std::logic_error("Tried to nest inside of the non-nest column");
    
    mNestedColumns.push_back(CSMWorld::NestedColumn(columnId, displayType, mFlags, this));
}

const CSMWorld::ColumnBase& CSMWorld::NestColumn::nestedColumn(int subColumn) const 
{
    if (!mCanNest)
        throw std::logic_error("Tried to access nested column of the non-nest column");
    
    return mNestedColumns.at(subColumn);
}

int CSMWorld::NestColumn::nestedColumnCount() const
{
    if (!mCanNest)
        throw std::logic_error("Tried to access number of the subcolumns in the non-nest column");
    
    return mNestedColumns.size();
}

CSMWorld::NestColumn::NestColumn(int columnId, Display displayType, int flags, bool canNest)
    : CSMWorld::ColumnBase(columnId, displayType, flags, canNest) {}

CSMWorld::NestedColumn::NestedColumn(int columnId, Display displayType, int flag, const CSMWorld::NestColumn* parent)
    : mParent(parent), CSMWorld::ColumnBase(columnId, displayType, flag) {}

bool CSMWorld::NestedColumn::isEditable() const
{
    return mParent->isEditable();
}
