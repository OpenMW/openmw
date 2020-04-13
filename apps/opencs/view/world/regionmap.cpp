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
#include "../../model/world/columns.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/commandmacro.hpp"

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
            mCreateCellsAction->setText ("Create one Cell");
        else
        {
            std::ostringstream stream;
            stream << "Create " << selectedNonExistentCells << " cells";
            mCreateCellsAction->setText (QString::fromUtf8 (stream.str().c_str()));
        }

        menu.addAction (mCreateCellsAction);
    }

    if (getSelectedCells().size()>0)
    {
        if (!mRegionId.empty())
        {
            mSetRegionAction->setText (QString::fromUtf8 (("Set Region to " + mRegionId).c_str()));
            menu.addAction (mSetRegionAction);
        }

        menu.addAction (mUnsetRegionAction);

        menu.addAction (mViewInTableAction);
    }

    if (selectionModel()->selectedIndexes().size()>0)
        menu.addAction (mViewAction);

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

void CSVWorld::RegionMap::setRegion (const std::string& regionId)
{
    QModelIndexList selected = getSelectedCells();

    QAbstractItemModel *regionModel = model();

    CSMWorld::IdTable *cellsModel = &dynamic_cast<CSMWorld::IdTable&> (*
        mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    QString regionId2 = QString::fromUtf8 (regionId.c_str());

    CSMWorld::CommandMacro macro (mDocument.getUndoStack(), selected.size()>1 ? tr ("Set Region") : "");

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        std::string cellId = regionModel->data (*iter, CSMWorld::RegionMap::Role_CellId).
            toString().toUtf8().constData();

        QModelIndex index = cellsModel->getModelIndex (cellId,
            cellsModel->findColumnIndex (CSMWorld::Columns::ColumnId_Region));

        macro.push (new CSMWorld::ModifyCommand (*cellsModel, index, regionId2));
    }
}

CSVWorld::RegionMap::RegionMap (const CSMWorld::UniversalId& universalId,
    CSMDoc::Document& document, QWidget *parent)
:  DragRecordTable(document, parent)
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

    mSetRegionAction = new QAction (tr ("Set Region"), this);
    connect (mSetRegionAction, SIGNAL (triggered()), this, SLOT (setRegion()));
    addAction (mSetRegionAction);

    mUnsetRegionAction = new QAction (tr ("Unset Region"), this);
    connect (mUnsetRegionAction, SIGNAL (triggered()), this, SLOT (unsetRegion()));
    addAction (mUnsetRegionAction);

    mViewAction = new QAction (tr ("View Cells"), this);
    connect (mViewAction, SIGNAL (triggered()), this, SLOT (view()));
    addAction (mViewAction);

    mViewInTableAction = new QAction (tr ("View Cells in Table"), this);
    connect (mViewInTableAction, SIGNAL (triggered()), this, SLOT (viewInTable()));
    addAction (mViewInTableAction);

    setAcceptDrops(true);
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

    CSMWorld::IdTable *cellsModel = &dynamic_cast<CSMWorld::IdTable&> (*
        mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    CSMWorld::CommandMacro macro (mDocument.getUndoStack(), selected.size()>1 ? tr ("Create cells"): "");

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        std::string cellId = model()->data (*iter, CSMWorld::RegionMap::Role_CellId).
            toString().toUtf8().constData();

        macro.push (new CSMWorld::CreateCommand (*cellsModel, cellId));
    }
}

void CSVWorld::RegionMap::setRegion()
{
    if (mEditLock)
        return;

    setRegion (mRegionId);
}

void CSVWorld::RegionMap::unsetRegion()
{
    if (mEditLock)
        return;

    setRegion ("");
}

void CSVWorld::RegionMap::view()
{
    std::ostringstream hint;
    hint << "c:";

    QModelIndexList selected = selectionModel()->selectedIndexes();

    bool first = true;

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        std::string cellId = model()->data (*iter, CSMWorld::RegionMap::Role_CellId).
            toString().toUtf8().constData();

        if (first)
            first = false;
        else
            hint << "; ";

        hint << cellId;
    }

    emit editRequest (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Scene, ESM::CellId::sDefaultWorldspace),
        hint.str());
}

void CSVWorld::RegionMap::viewInTable()
{
    std::ostringstream hint;
    hint << "f:!or(";

    QModelIndexList selected = getSelectedCells();

    bool first = true;

    for (QModelIndexList::const_iterator iter (selected.begin()); iter!=selected.end(); ++iter)
    {
        std::string cellId = model()->data (*iter, CSMWorld::RegionMap::Role_CellId).
            toString().toUtf8().constData();

        if (first)
            first = false;
        else
            hint << ",";

        hint << "string(ID,\"" << cellId << "\")";
    }

    hint << ")";

    emit editRequest (CSMWorld::UniversalId::Type_Cells, hint.str());
}

void CSVWorld::RegionMap::mouseMoveEvent (QMouseEvent* event)
{
    startDragFromTable(*this);
}

std::vector< CSMWorld::UniversalId > CSVWorld::RegionMap::getDraggedRecords() const
{
    QModelIndexList selected(getSelectedCells(true, false));
    std::vector<CSMWorld::UniversalId> ids;
    for (const QModelIndex& it : selected)
    {
        ids.push_back(
            CSMWorld::UniversalId(
                CSMWorld::UniversalId::Type_Cell,
                model()->data(it, CSMWorld::RegionMap::Role_CellId).toString().toUtf8().constData()));
    }
    selected = getSelectedCells(false, true);
    for (const QModelIndex& it : selected)
    {
        ids.push_back(
            CSMWorld::UniversalId(
                CSMWorld::UniversalId::Type_Cell_Missing,
                model()->data(it, CSMWorld::RegionMap::Role_CellId).toString().toUtf8().constData()));
    }
    return ids;
}

void CSVWorld::RegionMap::dropEvent (QDropEvent* event)
{
    QModelIndex index = indexAt (event->pos());

    bool exists = QTableView::model()->data(index, Qt::BackgroundRole)!=QBrush (Qt::DiagCrossPattern);

    if (!index.isValid() || !exists)
    {
        return;
    }

    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument(mDocument) && mime->holdsType(CSMWorld::UniversalId::Type_Region))
    {
        CSMWorld::UniversalId record (mime->returnMatching (CSMWorld::UniversalId::Type_Region));

        QAbstractItemModel *regionModel = model();

        CSMWorld::IdTable *cellsModel = &dynamic_cast<CSMWorld::IdTable&> (*
            mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

        std::string cellId(regionModel->data (index, CSMWorld::RegionMap::Role_CellId).
            toString().toUtf8().constData());

        QModelIndex index2(cellsModel->getModelIndex (cellId,
            cellsModel->findColumnIndex (CSMWorld::Columns::ColumnId_Region)));

        mDocument.getUndoStack().push(new CSMWorld::ModifyCommand
                                        (*cellsModel, index2, QString::fromUtf8(record.getId().c_str())));

        mRegionId = record.getId();
    }
}
