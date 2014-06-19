
#include "columnbase.hpp"

#include "columns.hpp"

#include <cassert>

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

bool CSMWorld::ColumnBase::canHaveNestedColumns() const
{
    return mCanNest;
}

std::string CSMWorld::ColumnBase::getNestedColumnTitle(int columnNumber) const
{
    return Columns::getName (mDisplayType, columnNumber);
}

void CSMWorld::ColumnBase::addNestedColumnDisplay(CSMWorld::ColumnBase::Display displayDefinition)
{
    assert (canHaveNestedColumns());
    
    mNestedDisplayType.push_back(displayDefinition);
}
