#include "masterproxymodel.hpp"
#include <QMimeData>
#include <QDebug>

EsxModel::MasterProxyModel::MasterProxyModel(QObject *parent, QAbstractTableModel* model) :
    QSortFilterProxyModel(parent)
{
    setFilterRegExp(QString("game"));
    setFilterRole (Qt::UserRole);

    if (model)
         setSourceModel (model);
}
