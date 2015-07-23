
#include "idtableproxymodel.hpp"

#include <vector>

#include "idtablebase.hpp"

void CSMWorld::IdTableProxyModel::updateColumnMap()
{
    mColumnMap.clear();

    if (mFilter)
    {
        std::vector<int> columns = mFilter->getReferencedColumns();

        const IdTableBase& table = dynamic_cast<const IdTableBase&> (*sourceModel());

        for (std::vector<int>::const_iterator iter (columns.begin()); iter!=columns.end(); ++iter)
            mColumnMap.insert (std::make_pair (*iter,
                table.searchColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (*iter))));
    }
}

bool CSMWorld::IdTableProxyModel::filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent)
    const
{
    // It is not possible to use filterAcceptsColumn() and check for
    // sourceModel()->headerData (sourceColumn, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags)
    // because the sourceColumn parameter excludes the hidden columns, i.e. wrong columns can
    // be rejected.  Workaround by disallowing tree branches (nested columns), which are not meant
    // to be visible, from the filter.
    if (sourceParent.isValid())
        return false;

    if (!mFilter)
        return true;

    return mFilter->test (
        dynamic_cast<IdTableBase&> (*sourceModel()), sourceRow, mColumnMap);
}

CSMWorld::IdTableProxyModel::IdTableProxyModel (QObject *parent)
: QSortFilterProxyModel (parent)
{
    setSortCaseSensitivity (Qt::CaseInsensitive);
}

QModelIndex CSMWorld::IdTableProxyModel::getModelIndex (const std::string& id, int column) const
{
    return mapFromSource (dynamic_cast<IdTableBase&> (*sourceModel()).getModelIndex (id, column));
}

void CSMWorld::IdTableProxyModel::setFilter (const boost::shared_ptr<CSMFilter::Node>& filter)
{
    beginResetModel();
    mFilter = filter;
    updateColumnMap();
    endResetModel();
}

bool CSMWorld::IdTableProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    Columns::ColumnId id = static_cast<Columns::ColumnId>(left.data(ColumnBase::Role_ColumnId).toInt());
    EnumColumnCache::const_iterator valuesIt = mEnumColumnCache.find(id);
    if (valuesIt == mEnumColumnCache.end())
    {
        if (Columns::hasEnums(id))
        {
            valuesIt = mEnumColumnCache.insert(std::make_pair(id, Columns::getEnums(id))).first;
        }
    }

    if (valuesIt != mEnumColumnCache.end())
    {
        return valuesIt->second[left.data().toInt()] < valuesIt->second[right.data().toInt()];
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

void CSMWorld::IdTableProxyModel::refreshFilter()
{
    updateColumnMap();
    invalidateFilter();
}
