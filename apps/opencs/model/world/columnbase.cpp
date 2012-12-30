
#include "columnbase.hpp"

CSMWorld::ColumnBase::ColumnBase (const std::string& title, int flags)
: mTitle (title), mFlags (flags)
{}

CSMWorld::ColumnBase::~ColumnBase() {}

bool CSMWorld::ColumnBase::isUserEditable() const
{
    return isEditable();
}