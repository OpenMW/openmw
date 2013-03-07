#include "pluginsproxymodel.hpp"

PluginsProxyModel::PluginsProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

PluginsProxyModel::~PluginsProxyModel()
{
}

QVariant PluginsProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Vertical || role != Qt::DisplayRole)
        return QSortFilterProxyModel::headerData(section, orientation, role);
    return section + 1;
}
