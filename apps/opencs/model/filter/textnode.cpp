#include "textnode.hpp"

#include <sstream>
#include <stdexcept>

#include <QRegExp>

#include "../world/columns.hpp"
#include "../world/idtablebase.hpp"

CSMFilter::TextNode::TextNode (int columnId, const std::string& text)
: mColumnId (columnId), mText (text)
{}

bool CSMFilter::TextNode::test (const CSMWorld::IdTableBase& table, int row,
    const std::map<int, int>& columns) const
{
    const std::map<int, int>::const_iterator iter = columns.find (mColumnId);

    if (iter==columns.end())
        throw std::logic_error ("invalid column in text node test");

    if (iter->second==-1)
        return true;

    QModelIndex index = table.index (row, iter->second);

    QVariant data = table.data (index);

    QString string;

    if (data.type()==QVariant::String)
    {
        string = data.toString();
    }
    else if ((data.type()==QVariant::Int || data.type()==QVariant::UInt) &&
        CSMWorld::Columns::hasEnums (static_cast<CSMWorld::Columns::ColumnId> (mColumnId)))
    {
        int value = data.toInt();

        std::vector<std::pair<int,std::string>> enums =
            CSMWorld::Columns::getEnums (static_cast<CSMWorld::Columns::ColumnId> (mColumnId));

        if (value>=0 && value<static_cast<int> (enums.size()))
            string = QString::fromUtf8 (enums[value].second.c_str());
    }
    else if (data.type()==QVariant::Bool)
    {
        string = data.toBool() ? "true" : "false";
    }
    else if (mText.empty() && !data.isValid())
        return true;
    else
        return false;

    /// \todo make pattern syntax configurable
    QRegExp regExp (QString::fromUtf8 (mText.c_str()), Qt::CaseInsensitive);

    return regExp.exactMatch (string);
}

std::vector<int> CSMFilter::TextNode::getReferencedColumns() const
{
    return std::vector<int> (1, mColumnId);
}

std::string CSMFilter::TextNode::toString (bool numericColumns) const
{
    std::ostringstream stream;

    stream << "text (";

    if (numericColumns)
        stream << mColumnId;
    else
        stream
            << "\""
            << CSMWorld::Columns::getName (static_cast<CSMWorld::Columns::ColumnId> (mColumnId))
            << "\"";

    stream << ", \"" << mText << "\")";

    return stream.str();
}
