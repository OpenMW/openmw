#include "masterproxymodel.hpp"

MasterProxyModel::MasterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

QVariant MasterProxyModel::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data (index, role);
}
