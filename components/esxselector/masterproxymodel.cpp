#include "masterproxymodel.hpp"

FileOrderList::MasterProxyModel::MasterProxyModel(QObject *parent, QAbstractTableModel* model) :
    QSortFilterProxyModel(parent)
{
    setFilterRegExp(QString("game"));
    setFilterRole (Qt::UserRole);

    if (model)
        setSourceModel (model);
}

QVariant FileOrderList::MasterProxyModel::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data (index, role);
}
