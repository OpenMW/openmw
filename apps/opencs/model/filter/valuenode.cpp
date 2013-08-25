
#include "valuenode.hpp"

#include <sstream>
#include <stdexcept>

#include "../world/columns.hpp"
#include "../world/idtable.hpp"

CSMFilter::ValueNode::ValueNode (int columnId,
    double lower, double upper, bool min, bool max)
: mColumnId (columnId), mLower (lower), mUpper (upper), mMin (min), mMax (max)
{}

bool CSMFilter::ValueNode::test (const CSMWorld::IdTable& table, int row,
    const std::map<int, int>& columns) const
{
    const std::map<int, int>::const_iterator iter = columns.find (mColumnId);

    if (iter==columns.end())
        throw std::logic_error ("invalid column in test value test");

    if (iter->second==-1)
        return true;

    QModelIndex index = table.index (row, iter->second);

    QVariant data = table.data (index);

    if (data.type()!=QVariant::Double && data.type()!=QVariant::Bool && data.type()!=QVariant::Int &&
        data.type()!=QVariant::UInt)
        return false;

    double value = data.toDouble();

    if (mLower==mUpper && mMin && mMax)
        return value==mLower;

    return (mMin ? value>=mLower : value>mLower) && (mMax ? value<=mUpper : value<mUpper);
}

std::vector<int> CSMFilter::ValueNode::getReferencedColumns() const
{
    return std::vector<int> (1, mColumnId);
}

std::string CSMFilter::ValueNode::toString (bool numericColumns) const
{
    std::ostringstream stream;

    stream << "value (";

    if (numericColumns)
        stream << mColumnId;
    else
        stream
            << "\""
            << CSMWorld::Columns::getName (static_cast<CSMWorld::Columns::ColumnId> (mColumnId))
            << "\"";

    stream << ", \"";

    if (mLower==mUpper && mMin && mMax)
        stream << mLower;
    else
        stream << (mMin ? "[" : "(") << mLower << ", " << mUpper << (mMax ? "]" : ")");

    stream << ")";

    return stream.str();
}