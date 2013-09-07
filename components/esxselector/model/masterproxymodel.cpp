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
        //connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(slotSourceModelChanged(QModelIndex, QModelIndex)));
}
/*
QVariant EsxModel::MasterProxyModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
        return QSortFilterProxyModel::data (index, role);

    return 0;
}
*/
void EsxModel::MasterProxyModel::slotSourceModelChanged(QModelIndex topLeft, QModelIndex botRight)
{
    qDebug() << "source data changed.. updating master proxy";
    emit dataChanged(index(0,0), index(rowCount()-1,0));

    int curRow = -1;
/*
    for (int i = 0; i < rowCount() - 1; ++i)
    {
         if (index(i,0).data(Qt::CheckState) == Qt::Checked)
         {
             curRow = i;
             break;
         }
    }

    reset();
*/
    if (curRow != -1);
     //   index(curRow, 0).setDataQt::CheckState)
}
