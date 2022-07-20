#include "reportmodel.hpp"

#include <stdexcept>
#include <sstream>

#include "../world/columns.hpp"

CSMTools::ReportModel::ReportModel (bool fieldColumn, bool severityColumn)
: mColumnField (-1), mColumnSeverity (-1)
{
    int index = 3;

    if (severityColumn)
        mColumnSeverity = index++;

    if (fieldColumn)
        mColumnField = index++;

    mColumnDescription = index;
}

int CSMTools::ReportModel::rowCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(mRows.size());
}

int CSMTools::ReportModel::columnCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return mColumnDescription+1;
}

QVariant CSMTools::ReportModel::data (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole && role!=Qt::UserRole)
        return QVariant();

    switch (index.column())
    {
        case Column_Type:

            if(role == Qt::UserRole)
                return QString::fromUtf8 (
                    mRows.at (index.row()).mId.getTypeName().c_str());
            else
                return static_cast<int> (mRows.at (index.row()).mId.getType());

        case Column_Id:
        {
            CSMWorld::UniversalId id = mRows.at (index.row()).mId;

            if (id.getArgumentType()==CSMWorld::UniversalId::ArgumentType_Id)
                return QString::fromUtf8 (id.getId().c_str());

            return QString ("-");
        }

        case Column_Hint:

            return QString::fromUtf8 (mRows.at (index.row()).mHint.c_str());
    }

    if (index.column()==mColumnDescription)
        return QString::fromUtf8 (mRows.at (index.row()).mMessage.c_str());

    if (index.column()==mColumnField)
    {
        std::string field;

        std::istringstream stream (mRows.at (index.row()).mHint);

        char type, ignore;
        int fieldIndex;

        if ((stream >> type >> ignore >> fieldIndex) && (type=='r' || type=='R'))
        {
            field = CSMWorld::Columns::getName (
                static_cast<CSMWorld::Columns::ColumnId> (fieldIndex));
        }

        return QString::fromUtf8 (field.c_str());
    }

    if (index.column()==mColumnSeverity)
    {
        return QString::fromUtf8 (
            CSMDoc::Message::toString (mRows.at (index.row()).mSeverity).c_str());
    }

    return QVariant();
}

QVariant CSMTools::ReportModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (orientation==Qt::Vertical)
        return QVariant();

    switch (section)
    {
        case Column_Type: return "Type";
        case Column_Id: return "ID";
    }

    if (section==mColumnDescription)
        return "Description";

    if (section==mColumnField)
        return "Field";

    if (section==mColumnSeverity)
        return "Severity";

    return "-";
}

bool CSMTools::ReportModel::removeRows (int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    if (count>0)
    {
        beginRemoveRows (parent, row, row+count-1);

        mRows.erase (mRows.begin()+row, mRows.begin()+row+count);

        endRemoveRows();
    }

    return true;
}

void CSMTools::ReportModel::add (const CSMDoc::Message& message)
{
    beginInsertRows (QModelIndex(), static_cast<int>(mRows.size()), static_cast<int>(mRows.size()));

    mRows.push_back (message);

    endInsertRows();
}

void CSMTools::ReportModel::flagAsReplaced (int index)
{
    CSMDoc::Message& line = mRows.at (index);
    std::string hint = line.mHint;

    if (hint.empty() || hint[0]!='R')
        throw std::logic_error ("trying to flag message as replaced that is not replaceable");

    hint[0] = 'r';

    line.mHint = hint;

    emit dataChanged (this->index (index, 0), this->index (index, columnCount()));
}

const CSMWorld::UniversalId& CSMTools::ReportModel::getUniversalId (int row) const
{
    return mRows.at (row).mId;
}

std::string CSMTools::ReportModel::getHint (int row) const
{
    return mRows.at (row).mHint;
}

void CSMTools::ReportModel::clear()
{
    if (!mRows.empty())
    {
        beginRemoveRows (QModelIndex(), 0, static_cast<int>(mRows.size())-1);
        mRows.clear();
        endRemoveRows();
    }
}

int CSMTools::ReportModel::countErrors() const
{
    int count = 0;

    for (std::vector<CSMDoc::Message>::const_iterator iter (mRows.begin());
        iter!=mRows.end(); ++iter)
        if (iter->mSeverity==CSMDoc::Message::Severity_Error ||
            iter->mSeverity==CSMDoc::Message::Severity_SeriousError)
            ++count;

    return count;
}
