
#include "idtableproxymodel.hpp"

#include <map>
#include <vector>

#include "idtable.hpp"

bool CSMWorld::IdTableProxyModel::filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent)
    const
{
    if (!mFilter)
        return true;

    std::map<std::string, const CSMFilter::Node *> otherFilters; /// \todo get other filters;
    std::map<int, int> columns; /// \todo get columns

    return mFilter->test (dynamic_cast<IdTable&> (*sourceModel()), sourceRow, otherFilters, columns, mUserValue);
}

CSMWorld::IdTableProxyModel::IdTableProxyModel (QObject *parent)
: QSortFilterProxyModel (parent)
{}

QModelIndex CSMWorld::IdTableProxyModel::getModelIndex (const std::string& id, int column) const
{
    return mapFromSource (dynamic_cast<IdTable&> (*sourceModel()).getModelIndex (id, column));
}

void CSMWorld::IdTableProxyModel::setFilter (const boost::shared_ptr<CSMFilter::Node>& filter,
    const std::string& userValue)
{
    mFilter = filter;
    mUserValue = userValue;
    invalidateFilter();
}