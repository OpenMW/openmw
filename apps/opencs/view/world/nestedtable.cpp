#include "nestedtable.hpp"
#include "../../model/world/nestedtablemodel.hpp"
#include "../../model/world/universalid.hpp"
#include "util.hpp"

#include <QHeaderView>
#include <QContextMenuEvent>

CSVWorld::NestedTable::NestedTable(QUndoStack& undoStack,
                                   CSMWorld::NestedTableModel* model,
                                   QWidget* parent)
    : QTableView(parent),
      mUndoStack(undoStack)
{

    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    verticalHeader()->hide();

    int columns = model->columnCount(QModelIndex());

    for(int i = 0 ; i < columns; ++i)
    {
        CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display> (
            model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());
        
        CommandDelegate *delegate = CommandDelegateFactoryCollection::get().makeDelegate(display,
                                                                                         undoStack,
                                                                                         this);
        
        setItemDelegateForColumn(i, delegate);
    }

    setModel(model);
    setAcceptDrops(true);
}

void CSVWorld::NestedTable::dragEnterEvent(QDragEnterEvent *event)
{
}

void CSVWorld::NestedTable::dragMoveEvent(QDragMoveEvent *event)
{
}

void CSVWorld::NestedTable::contextMenuEvent (QContextMenuEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();
}
