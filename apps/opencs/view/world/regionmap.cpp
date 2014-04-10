
#include "regionmap.hpp"

#include <algorithm>
#include <set>

#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>

#include "../../model/world/regionmap.hpp"

void CSVWorld::RegionMap::contextMenuEvent (QContextMenuEvent *event)
{
    QMenu menu (this);

    if (getUnselectedCells().size()>0)
        menu.addAction (mSelectAllAction);

    if (selectionModel()->selectedIndexes().size()>0)
        menu.addAction (mClearSelectionAction);

    if (getMissingRegionCells().size()>0)
        menu.addAction (mSelectRegionsAction);

    menu.exec (event->globalPos());
}

QModelIndexList CSVWorld::RegionMap::getUnselectedCells() const
{
     const QAbstractItemModel *model = QTableView::model();

    int rows = model->rowCount();
    int columns = model->columnCount();

    QModelIndexList selected = selectionModel()->selectedIndexes();
    std::sort (selected.begin(), selected.end());

    QModelIndexList all;

    for (int y=0; y<rows; ++y)
        for (int x=0; x<columns; ++x)
        {
            QModelIndex index = model->index (y, x);
            if (model->data (index, Qt::BackgroundRole)!=QBrush (Qt::DiagCrossPattern))
                all.push_back (index);
        }

    std::sort (all.begin(), all.end());

    QModelIndexList list;

    std::set_difference (all.begin(), all.end(), selected.begin(), selected.end(),
        std::back_inserter (list));

    return list;
}

QModelIndexList CSVWorld::RegionMap::getMissingRegionCells() const
{
    const QAbstractItemModel *model = QTableView::model();

    QModelIndexList selected = selectionModel()->selectedIndexes();

    std::set<std::string> regions;

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        std::string region =
            model->data (*iter, CSMWorld::RegionMap::Role_Region).toString().toUtf8().constData();

        if (!region.empty())
            regions.insert (region);
    }

    QModelIndexList list;

    QModelIndexList unselected = getUnselectedCells();

    for (QModelIndexList::const_iterator iter (unselected.begin()); iter!=unselected.end(); ++iter)
    {
        std::string region =
            model->data (*iter, CSMWorld::RegionMap::Role_Region).toString().toUtf8().constData();

        if (!region.empty() && regions.find (region)!=regions.end())
            list.push_back (*iter);
    }

    return list;
}

CSVWorld::RegionMap::RegionMap (QAbstractItemModel *model, QWidget *parent)
: QTableView (parent)
{
    verticalHeader()->hide();
    horizontalHeader()->hide();

    setSelectionMode (QAbstractItemView::ExtendedSelection);

    setModel (model);

    resizeColumnsToContents();
    resizeRowsToContents();

    mSelectAllAction = new QAction (tr ("Select All"), this);
    connect (mSelectAllAction, SIGNAL (triggered()), this, SLOT (selectAll()));
    addAction (mSelectAllAction);

    mClearSelectionAction = new QAction (tr ("Clear Selection"), this);
    connect (mClearSelectionAction, SIGNAL (triggered()), this, SLOT (clearSelection()));
    addAction (mClearSelectionAction);

    mSelectRegionsAction = new QAction (tr ("Select Regions"), this);
    connect (mSelectRegionsAction, SIGNAL (triggered()), this, SLOT (selectRegions()));
    addAction (mSelectRegionsAction);
}

void CSVWorld::RegionMap::setEditLock (bool locked)
{

}

void CSVWorld::RegionMap::selectAll()
{
    QModelIndexList unselected = getUnselectedCells();

    for (QModelIndexList::const_iterator iter (unselected.begin()); iter!=unselected.end(); ++iter)
        selectionModel()->select (*iter, QItemSelectionModel::Select);
}

void CSVWorld::RegionMap::clearSelection()
{
    selectionModel()->clearSelection();
}

void CSVWorld::RegionMap::selectRegions()
{
    QModelIndexList unselected = getMissingRegionCells();

    for (QModelIndexList::const_iterator iter (unselected.begin()); iter!=unselected.end(); ++iter)
        selectionModel()->select (*iter, QItemSelectionModel::Select);
}