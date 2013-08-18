#include "pluginsproxymodel.hpp"
#include "datafilesmodel.hpp"

EsxModel::PluginsProxyModel::PluginsProxyModel(QObject *parent, DataFilesModel *model) :
    QSortFilterProxyModel(parent)
{
    setFilterRegExp (QString("addon"));
    setFilterRole (Qt::UserRole);
    setDynamicSortFilter (true);

    if (model)
        setSourceModel (model);
}

EsxModel::PluginsProxyModel::~PluginsProxyModel()
{
}

QVariant EsxModel::PluginsProxyModel::data(const QModelIndex &index, int role) const
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
