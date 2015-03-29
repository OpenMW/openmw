
#include "reportmodel.hpp"

#include <stdexcept>

CSMTools::ReportModel::Line::Line (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint)
: mId (id), mMessage (message), mHint (hint)
{}


int CSMTools::ReportModel::rowCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return mRows.size();
}

int CSMTools::ReportModel::columnCount (const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant CSMTools::ReportModel::data (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (index.column()==0)
        return static_cast<int> (mRows.at (index.row()).mId.getType());

    if (index.column()==1)
        return QString::fromUtf8 (mRows.at (index.row()).mMessage.c_str());

    return QString::fromUtf8 (mRows.at (index.row()).mHint.c_str());
}

QVariant CSMTools::ReportModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (orientation==Qt::Vertical)
        return QVariant();

    if (section==0)
        return "Type";

    if (section==1)
        return "Description";

    return "Hint";
}

bool CSMTools::ReportModel::removeRows (int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    mRows.erase (mRows.begin()+row, mRows.begin()+row+count);

    return true;
}

void CSMTools::ReportModel::add (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint)
{
    beginInsertRows (QModelIndex(), mRows.size(), mRows.size());
    
    mRows.push_back (Line (id, message, hint));

    endInsertRows();
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
        beginRemoveRows (QModelIndex(), 0, mRows.size()-1);
        mRows.clear();
        endRemoveRows();
    }
}
