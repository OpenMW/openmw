
#include "columnbase.hpp"

#include "columns.hpp"

CSMWorld::ColumnBase::ColumnBase (int columnId, Display displayType, int flags)
: mColumnId (columnId), mDisplayType (displayType), mFlags (flags)
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