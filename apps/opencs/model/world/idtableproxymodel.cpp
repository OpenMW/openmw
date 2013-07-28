
#include "idtableproxymodel.hpp"

#include "idtable.hpp"

CSMWorld::IdTableProxyModel::IdTableProxyModel (QObject *parent)
: QSortFilterProxyModel (parent)
{}

QModelIndex CSMWorld::IdTableProxyModel::getModelIndex (const std::string& id, int column) const
{
    return mapFromSource (dynamic_cast<IdTable&> (*sourceModel()).getModelIndex (id, column));
}