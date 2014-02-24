
#include "idtableproxymodel.hpp"

#include <vector>

#include "idtable.hpp"

void CSMWorld::IdTableProxyModel::updateColumnMap()
{
    mColumnMap.clear();

    if (mFilter)
    {
        std::vector<int> columns = mFilter->getReferencedColumns();

        const IdTable& table = dynamic_cast<const IdTable&> (*sourceModel());

        for (std::vector<int>::const_iterator iter (columns.begin()); iter!=columns.end(); ++iter)
            mColumnMap.insert (std::make_pair (*iter,
                table.searchColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (*iter))));
    }
}

bool CSMWorld::IdTableProxyModel::filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent)
    const
{
    if (!mFilter)
        return true;

    return mFilter->test (
        dynamic_cast<IdTable&> (*sourceModel()), sourceRow, mColumnMap);
}

CSMWorld::IdTableProxyModel::IdTableProxyModel (QObject *parent)
: QSortFilterProxyModel (parent)
{
    setSortCaseSensitivity (Qt::CaseInsensitive);
}

QModelIndex CSMWorld::IdTableProxyModel::getModelIndex (const std::string& id, int column) const
{
    return mapFromSource (dynamic_cast<IdTable&> (*sourceModel()).getModelIndex (id, column));
}

void CSMWorld::IdTableProxyModel::setFilter (const boost::shared_ptr<CSMFilter::Node>& filter)
{
    mFilter = filter;
    updateColumnMap();
    invalidateFilter();
}