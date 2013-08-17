#include "masterproxymodel.hpp"

MasterProxyModel::MasterProxyModel(QObject *parent, QAbstractTableModel* model) :
    QSortFilterProxyModel(parent)
{
    setFilterRegExp(QString("game"));
    setFilterRole (Qt::UserRole);

    if (model)
        setSourceModel (model);
}

QVariant MasterProxyModel::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data (index, role);
}
