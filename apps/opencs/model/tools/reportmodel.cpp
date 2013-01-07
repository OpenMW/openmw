
#include "reportmodel.hpp"

#include <stdexcept>

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

    return 2;
}

QVariant CSMTools::ReportModel::data (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (index.column()==0)
        return static_cast<int> (mRows.at (index.row()).first.getType());
    else
        return mRows.at (index.row()).second.c_str();
}

QVariant CSMTools::ReportModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    if (orientation==Qt::Vertical)
        return QVariant();

    return tr (section==0 ? "Type" : "Description");
}

bool CSMTools::ReportModel::removeRows (int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    mRows.erase (mRows.begin()+row, mRows.begin()+row+count);

    return true;
}

void CSMTools::ReportModel::add (const std::string& row)
{
    std::string::size_type index = row.find ('|');

    if (index==std::string::npos)
        throw std::logic_error ("invalid report message");

    beginInsertRows (QModelIndex(), mRows.size(), mRows.size());

    mRows.push_back (std::make_pair (row.substr (0, index), row.substr (index+1)));

    endInsertRows();
}

const CSMWorld::UniversalId& CSMTools::ReportModel::getUniversalId (int row) const
{
    return mRows.at (row).first;
}