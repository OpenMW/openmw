
#include "columnbase.hpp"

CSMWorld::ColumnBase::ColumnBase (const std::string& title, Display displayType, int flags)
: mTitle (title), mDisplayType (displayType), mFlags (flags)
{}

CSMWorld::ColumnBase::~ColumnBase() {}

bool CSMWorld::ColumnBase::isUserEditable() const
{
    return isEditable();
}