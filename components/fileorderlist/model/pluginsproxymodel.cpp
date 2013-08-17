#include "pluginsproxymodel.hpp"
#include "datafilesmodel.hpp"

PluginsProxyModel::PluginsProxyModel(QObject *parent, DataFilesModel *model) :
    QSortFilterProxyModel(parent), mSourceModel (model)
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
    switch (role)
    {
        case Qt::CheckStateRole:
        {
        if (index.column() != 0)
                return QVariant();

            return mSourceModel->checkState(index);
        }
    };

    return QSortFilterProxyModel::data (index, role);
}
