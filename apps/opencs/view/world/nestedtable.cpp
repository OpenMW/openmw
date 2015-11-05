#include "nestedtable.hpp"

#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDebug>

#include "../../model/world/nestedtableproxymodel.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/commanddispatcher.hpp"

#include "tableeditidaction.hpp"
#include "util.hpp"

CSVWorld::NestedTable::NestedTable(CSMDoc::Document& document,
                                   CSMWorld::UniversalId id,
                                   CSMWorld::NestedTableProxyModel* model,
                                   QWidget* parent,
                                   bool editable,
                                   bool fixedRows)
    : DragRecordTable(document, parent),
      mAddNewRowAction(NULL),
      mRemoveRowAction(NULL),
      mEditIdAction(NULL),
      mModel(model)
{
    mDispatcher = new CSMWorld::CommandDispatcher (document, id, this);

    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    horizontalHeader()->setSectionResizeMode (QHeaderView::Interactive);
#else
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
#endif
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

    if (editable)
    {
        if (!fixedRows)
        {
            mAddNewRowAction = new QAction (tr ("Add new row"), this);

            connect(mAddNewRowAction, SIGNAL(triggered()),
                    this, SLOT(addNewRowActionTriggered()));

            mRemoveRowAction = new QAction (tr ("Remove row"), this);

            connect(mRemoveRowAction, SIGNAL(triggered()),
                    this, SLOT(removeRowActionTriggered()));
        }

        mEditIdAction = new TableEditIdAction(*this, this);
        connect(mEditIdAction, SIGNAL(triggered()), this, SLOT(editCell()));
    }
}

std::vector<CSMWorld::UniversalId> CSVWorld::NestedTable::getDraggedRecords() const
{
    // No drag support for nested tables
    return std::vector<CSMWorld::UniversalId>();
}

void CSVWorld::NestedTable::contextMenuEvent (QContextMenuEvent *event)
{
    if (!mEditIdAction)
        return;

    QModelIndexList selectedRows = selectionModel()->selectedRows();

    QMenu menu(this);

    int currentRow = rowAt(event->y());
    int currentColumn = columnAt(event->x());
    if (mEditIdAction->isValidIdCell(currentRow, currentColumn))
    {
        mEditIdAction->setCell(currentRow, currentColumn);
        menu.addAction(mEditIdAction);
        menu.addSeparator();
    }

    if (mAddNewRowAction && mRemoveRowAction)
    {
        if (selectionModel()->selectedRows().size() == 1)
            menu.addAction(mRemoveRowAction);

        menu.addAction(mAddNewRowAction);
    }

    menu.exec (event->globalPos());
}

void CSVWorld::NestedTable::removeRowActionTriggered()
{
    mDocument.getUndoStack().push(new CSMWorld::DeleteNestedCommand(*(mModel->model()),
                                                                    mModel->getParentId(),
                                                                    selectionModel()->selectedRows().begin()->row(),
                                                                    mModel->getParentColumn()));
}

void CSVWorld::NestedTable::addNewRowActionTriggered()
{
    mDocument.getUndoStack().push(new CSMWorld::AddNestedCommand(*(mModel->model()),
                                                                 mModel->getParentId(),
                                                                 selectionModel()->selectedRows().size(),
                                                                 mModel->getParentColumn()));
}

void CSVWorld::NestedTable::editCell()
{
    emit editRequest(mEditIdAction->getCurrentId(), "");
}
