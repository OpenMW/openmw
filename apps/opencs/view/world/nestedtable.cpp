#include "nestedtable.hpp"
#include "../../model/world/nestedtableproxymodel.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/commanddispatcher.hpp"
#include "util.hpp"

#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDebug>

CSVWorld::NestedTable::NestedTable(CSMDoc::Document& document,
                                   CSMWorld::UniversalId id,
                                   CSMWorld::NestedTableProxyModel* model,
                                   QWidget* parent)
    : QTableView(parent),
      mUndoStack(document.getUndoStack()),
      mModel(model)
{
    mDispatcher = new CSMWorld::CommandDispatcher (document, id, this);

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
                                                                                         mDispatcher,
                                                                                         document,
                                                                                         this);

        setItemDelegateForColumn(i, delegate);
    }

    setModel(model);

    setAcceptDrops(true);

    mAddNewRowAction = new QAction (tr ("Add new row"), this);

    connect(mAddNewRowAction, SIGNAL(triggered()),
            this, SLOT(addNewRowActionTriggered()));

    mRemoveRowAction = new QAction (tr ("Remove row"), this);

    connect(mRemoveRowAction, SIGNAL(triggered()),
            this, SLOT(removeRowActionTriggered()));
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

    QMenu menu(this);

    if (selectionModel()->selectedRows().size() == 1)
        menu.addAction(mRemoveRowAction);

    menu.addAction(mAddNewRowAction);

    menu.exec (event->globalPos());
}

void CSVWorld::NestedTable::removeRowActionTriggered()
{
    mUndoStack.push(new CSMWorld::DeleteNestedCommand(*(mModel->model()),
                                                      mModel->getParentId(),
                                                      selectionModel()->selectedRows().begin()->row(),
                                                      mModel->getParentColumn()));
}

void CSVWorld::NestedTable::addNewRowActionTriggered()
{
    mUndoStack.push(new CSMWorld::AddNestedCommand(*(mModel->model()),
                                                   mModel->getParentId(),
                                                   selectionModel()->selectedRows().size(),
                                                   mModel->getParentColumn()));
}
