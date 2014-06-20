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
    assert (mCanNest);

    return Columns::getName(static_cast<Columns::ColumnId>(mNestedColumnId[columnNumber]));
}

void CSMWorld::ColumnBase::addNestedColumnDisplay(CSMWorld::ColumnBase::Display displayDefinition)
{
    assert (mCanNest);

    mNestedDisplayType.push_back(displayDefinition);
}

void CSMWorld::ColumnBase::addNestedColumnId(int columnId)
{
    assert (mCanNest);

    mNestedColumnId.push_back(columnId);
}
