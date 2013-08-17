#include "pluginsproxymodel.hpp"

PluginsProxyModel::PluginsProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

PluginsProxyModel::~PluginsProxyModel()
{
}

QVariant PluginsProxyModel::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data (index, role);
}
