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
