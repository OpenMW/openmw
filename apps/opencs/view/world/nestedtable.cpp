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
                                   QWidget* parent,
                                   bool editable)
    : DragRecordTable(document, parent),
      mModel(model)
{
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    horizontalHeader()->setSectionResizeMode (QHeaderView::Interactive);
#else
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
#endif
    verticalHeader()->hide();

    int columns = model->columnCount(QModelIndex());

    setModel(model);

    setAcceptDrops(true);

    if (editable)
    {
        mDispatcher = new CSMWorld::CommandDispatcher (document, id, this);
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

        mAddNewRowAction = new QAction (tr ("Add new row"), this);

        connect(mAddNewRowAction, SIGNAL(triggered()),
                this, SLOT(addNewRowActionTriggered()));

        mRemoveRowAction = new QAction (tr ("Remove row"), this);

        connect(mRemoveRowAction, SIGNAL(triggered()),
                this, SLOT(removeRowActionTriggered()));
    }
}

std::vector<CSMWorld::UniversalId> CSVWorld::NestedTable::getDraggedRecords() const
{
    // No drag support for nested tables
    return std::vector<CSMWorld::UniversalId>();
}

void CSVWorld::NestedTable::contextMenuEvent (QContextMenuEvent *event)
{
    if (!mRemoveRowAction || !mAddNewRowAction)
        return;

    QModelIndexList selectedRows = selectionModel()->selectedRows();

    QMenu menu(this);

    if (selectionModel()->selectedRows().size() == 1)
        menu.addAction(mRemoveRowAction);

    menu.addAction(mAddNewRowAction);

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
