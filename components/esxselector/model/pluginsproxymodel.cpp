#include "pluginsproxymodel.hpp"
#include "contentmodel.hpp"
#include <QDebug>

EsxModel::PluginsProxyModel::PluginsProxyModel(QObject *parent, ContentModel *model) :
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

            return static_cast<ContentModel *>(sourceModel())->checkState(mapToSource(index));
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

bool EsxModel::PluginsProxyModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    bool success = QSortFilterProxyModel::removeRows(position, rows, parent);

    return success;
}
