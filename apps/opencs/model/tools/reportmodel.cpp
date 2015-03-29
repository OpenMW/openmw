
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

    return 4;
}

QVariant CSMTools::ReportModel::data (const QModelIndex & index, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (index.column())
    {
        case Column_Type:

            return static_cast<int> (mRows.at (index.row()).mId.getType());
        
        case Column_Id:
        {
            CSMWorld::UniversalId id = mRows.at (index.row()).mId;

            if (id.getArgumentType()==CSMWorld::UniversalId::ArgumentType_Id)
                return QString::fromUtf8 (id.getId().c_str());

            return QString ("-");
        }
        
        case Column_Description:

            return QString::fromUtf8 (mRows.at (index.row()).mMessage.c_str());
            
        case Column_Hint:

            return QString::fromUtf8 (mRows.at (index.row()).mHint.c_str());
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
        case Column_Description: return "Description";
    }

    return "-";
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
