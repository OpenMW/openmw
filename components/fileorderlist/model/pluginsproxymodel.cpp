#include "pluginsproxymodel.hpp"
#include "datafilesmodel.hpp"

PluginsProxyModel::PluginsProxyModel(QObject *parent, DataFilesModel *model) :
    QSortFilterProxyModel(parent)
{
    setFilterRegExp (QString("addon"));
    setFilterRole (Qt::UserRole);
    setDynamicSortFilter (true);

    if (model)
        setSourceModel (model);
}

PluginsProxyModel::~PluginsProxyModel()
{
}

QVariant PluginsProxyModel::data(const QModelIndex &index, int role) const
{
    switch (role)
    {
        case Qt::CheckStateRole:
        {
        if (index.column() != 0)
                return QVariant();

            return static_cast<DataFilesModel *>(sourceModel())->checkState(mapToSource(index));
        }
    };

    return QSortFilterProxyModel::data (index, role);
}
