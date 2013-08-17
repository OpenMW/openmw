#include "pluginsproxymodel.hpp"

PluginsProxyModel::PluginsProxyModel(QObject *parent, QAbstractTableModel *model) :
    QSortFilterProxyModel(parent)
{
    setFilterRegExp(QString("addon"));
    setFilterRole (Qt::UserRole);

    if (model)
        setSourceModel (model);
}

PluginsProxyModel::~PluginsProxyModel()
{
}

QVariant PluginsProxyModel::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data (index, role);
}
