#include "textnode.hpp"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <QRegularExpression>

#include "../world/columns.hpp"
#include "../world/idtablebase.hpp"

CSMFilter::TextNode::TextNode(int columnId, const std::string& text)
    : mColumnId(columnId)
    , mText(text)
    , mRegExp(QRegularExpression::anchoredPattern(QString::fromUtf8(mText.c_str())),
          QRegularExpression::CaseInsensitiveOption)
{
}

bool CSMFilter::TextNode::test(const CSMWorld::IdTableBase& table, int row, const std::map<int, int>& columns) const
{
    const std::map<int, int>::const_iterator iter = columns.find(mColumnId);

    if (iter == columns.end())
        throw std::logic_error("invalid column in text node test");

    if (iter->second == -1)
        return true;

    QModelIndex index = table.index(row, iter->second);

    QVariant data = table.data(index);

    QString string;

    if (data.typeId() == QMetaType::QString)
    {
        string = data.toString();
    }
    else if ((data.typeId() == QMetaType::Int || data.typeId() == QMetaType::UInt)
        && CSMWorld::Columns::hasEnums(static_cast<CSMWorld::Columns::ColumnId>(mColumnId)))
    {
        int value = data.toInt();

        std::vector<std::pair<int, std::string>> enums
            = CSMWorld::Columns::getEnums(static_cast<CSMWorld::Columns::ColumnId>(mColumnId));

        if (value >= 0 && value < static_cast<int>(enums.size()))
            string = QString::fromUtf8(enums[value].second.c_str());
    }
    else if (data.typeId() == QMetaType::Bool)
    {
        string = data.toBool() ? "true" : "false";
    }
    else if (mText.empty() && !data.isValid())
        return true;
    else
        return false;

    /// \todo make pattern syntax configurable
    QRegularExpressionMatch match = mRegExp.match(string);

    return match.hasMatch();
}

std::vector<int> CSMFilter::TextNode::getReferencedColumns() const
{
    return std::vector<int>(1, mColumnId);
}

std::string CSMFilter::TextNode::toString(bool numericColumns) const
{
    std::ostringstream stream;

    stream << "text (";

    if (numericColumns)
        stream << mColumnId;
    else
        stream << "\"" << CSMWorld::Columns::getName(static_cast<CSMWorld::Columns::ColumnId>(mColumnId)) << "\"";

    stream << ", \"" << mText << "\")";

    return stream.str();
}
