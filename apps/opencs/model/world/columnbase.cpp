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

void CSMWorld::NestableColumn::addColumn(CSMWorld::NestableColumn *column)
{
    mNestedColumns.push_back(column);
}

const CSMWorld::ColumnBase& CSMWorld::NestableColumn::nestedColumn(int subColumn) const
{
    if (mNestedColumns.empty())
        throw std::logic_error("Tried to access nested column of the non-nest column");

    return *mNestedColumns.at(subColumn);
}

CSMWorld::NestableColumn::NestableColumn(int columnId, CSMWorld::ColumnBase::Display displayType,
    int flag)
    : CSMWorld::ColumnBase(columnId, displayType, flag)
{}

CSMWorld::NestableColumn::~NestableColumn()
{
    for (unsigned int i = 0; i < mNestedColumns.size(); ++i)
    {
        delete mNestedColumns[i];
    }
}

bool CSMWorld::NestableColumn::hasChildren() const
{
    return !mNestedColumns.empty();
}

CSMWorld::NestedChildColumn::NestedChildColumn (int id,
    CSMWorld::ColumnBase::Display display, bool isEditable)
    : NestableColumn (id, display, CSMWorld::ColumnBase::Flag_Dialogue) , mIsEditable(isEditable)
{}

bool CSMWorld::NestedChildColumn::isEditable () const
{
    return mIsEditable;
}
