
#include "regionmap.hpp"

#include <algorithm>
#include <set>
#include <sstream>

#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>

#include "../../model/doc/document.hpp"

#include "../../model/world/regionmap.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/commands.hpp"

void CSVWorld::RegionMap::contextMenuEvent (QContextMenuEvent *event)
{
    QMenu menu (this);

    if (getUnselectedCells().size()>0)
        menu.addAction (mSelectAllAction);

    if (selectionModel()->selectedIndexes().size()>0)
        menu.addAction (mClearSelectionAction);

    if (getMissingRegionCells().size()>0)
        menu.addAction (mSelectRegionsAction);

    int selectedNonExistentCells = getSelectedCells (false, true).size();

    if (selectedNonExistentCells>0)
    {
        if (selectedNonExistentCells==1)
            mCreateCellsAction->setText ("Create one cell");
        else
        {
            std::ostringstream stream;
            stream << "Create " << selectedNonExistentCells << " cells";
            mCreateCellsAction->setText (QString::fromUtf8 (stream.str().c_str()));
        }

        menu.addAction (mCreateCellsAction);
    }

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

QModelIndexList CSVWorld::RegionMap::getSelectedCells (bool existent, bool nonExistent) const
{
    const QAbstractItemModel *model = QTableView::model();

    QModelIndexList selected = selectionModel()->selectedIndexes();

    QModelIndexList list;

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        bool exists = model->data (*iter, Qt::BackgroundRole)!=QBrush (Qt::DiagCrossPattern);

        if ((exists && existent) || (!exists && nonExistent))
             list.push_back (*iter);
    }

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

CSVWorld::RegionMap::RegionMap (const CSMWorld::UniversalId& universalId,
    CSMDoc::Document& document, QWidget *parent)
: QTableView (parent), mEditLock (false), mDocument (document)
{
    verticalHeader()->hide();
    horizontalHeader()->hide();

    setSelectionMode (QAbstractItemView::ExtendedSelection);

    setModel (document.getData().getTableModel (universalId));

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

    mCreateCellsAction = new QAction (tr ("Create Cells Action"), this);
    connect (mCreateCellsAction, SIGNAL (triggered()), this, SLOT (createCells()));
    addAction (mCreateCellsAction);
}

void CSVWorld::RegionMap::setEditLock (bool locked)
{
    mEditLock = locked;
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

void CSVWorld::RegionMap::createCells()
{
    if (mEditLock)
        return;

    QModelIndexList selected = getSelectedCells (false, true);

    QAbstractItemModel *regionModel = model();

    CSMWorld::IdTable *cellsModel = &dynamic_cast<CSMWorld::IdTable&> (*
        mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    if (selected.size()>1)
        mDocument.getUndoStack().beginMacro (tr ("Create cells"));

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        std::string cellId = regionModel->data (*iter, CSMWorld::RegionMap::Role_CellId).
            toString().toUtf8().constData();

        mDocument.getUndoStack().push (new CSMWorld::CreateCommand (*cellsModel, cellId));
    }

    if (selected.size()>1)
        mDocument.getUndoStack().endMacro();
}