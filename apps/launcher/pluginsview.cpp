#include <QSortFilterProxyModel>

#include "pluginsview.hpp"

PluginsView::PluginsView(QWidget *parent) : QTableView(parent)
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDropIndicatorShown(true);
    setDragDropOverwriteMode(false);
    setContextMenuPolicy(Qt::CustomContextMenu);

}

void PluginsView::startDrag(Qt::DropActions supportedActions)
{
    selectionModel()->select( selectionModel()->selection(),
                              QItemSelectionModel::Select | QItemSelectionModel::Rows );
    QAbstractItemView::startDrag( supportedActions );
}

void PluginsView::setModel(QSortFilterProxyModel *model)
{
    QTableView::setModel(model);

    qRegisterMetaType< QVector<QPersistentModelIndex> >();

    connect(model->sourceModel(), SIGNAL(indexesDropped(QVector<QPersistentModelIndex>)),
            this, SLOT(selectIndexes(QVector<QPersistentModelIndex>)), Qt::QueuedConnection);
}

void PluginsView::selectIndexes( QVector<QPersistentModelIndex> aIndexes )
{
    selectionModel()->clearSelection();
    foreach( QPersistentModelIndex pIndex, aIndexes )
        selectionModel()->select( pIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows );
}
