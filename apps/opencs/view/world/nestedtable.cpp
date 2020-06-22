#include "nestedtable.hpp"

#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDebug>

#include "../../model/prefs/shortcut.hpp"

#include "../../model/world/nestedtableproxymodel.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/commanddispatcher.hpp"
#include "../../model/world/commandmacro.hpp"

#include "tableeditidaction.hpp"
#include "util.hpp"

CSVWorld::NestedTable::NestedTable(CSMDoc::Document& document,
                                   CSMWorld::UniversalId id,
                                   CSMWorld::NestedTableProxyModel* model,
                                   QWidget* parent,
                                   bool editable,
                                   bool fixedRows)
    : DragRecordTable(document, parent),
      mAddNewRowAction(nullptr),
      mRemoveRowAction(nullptr),
      mEditIdAction(nullptr),
      mModel(model)
{
    mDispatcher = new CSMWorld::CommandDispatcher (document, id, this);

    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    horizontalHeader()->setSectionResizeMode (QHeaderView::Interactive);
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
            CSMPrefs::Shortcut* addRowShortcut = new CSMPrefs::Shortcut("table-add", this);
            addRowShortcut->associateAction(mAddNewRowAction);

            mRemoveRowAction = new QAction (tr ("Remove rows"), this);
            connect(mRemoveRowAction, SIGNAL(triggered()),
                    this, SLOT(removeRowActionTriggered()));
            CSMPrefs::Shortcut* removeRowShortcut = new CSMPrefs::Shortcut("table-remove", this);
            removeRowShortcut->associateAction(mRemoveRowAction);
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
        menu.addAction(mAddNewRowAction);
        menu.addAction(mRemoveRowAction);
    }

    menu.exec (event->globalPos());
}

void CSVWorld::NestedTable::removeRowActionTriggered()
{
    CSMWorld::CommandMacro macro(mDocument.getUndoStack(),
        selectionModel()->selectedRows().size() > 1 ? tr("Remove rows") : "");

    // Remove rows in reverse order
    for (int i = selectionModel()->selectedRows().size() - 1; i >= 0; --i)
    {
        macro.push(new CSMWorld::DeleteNestedCommand(*(mModel->model()), mModel->getParentId(),
            selectionModel()->selectedRows()[i].row(), mModel->getParentColumn()));
    }
}

void CSVWorld::NestedTable::addNewRowActionTriggered()
{
    int row = 0;

    if (!selectionModel()->selectedRows().empty())
        row = selectionModel()->selectedRows().back().row() + 1;

    mDocument.getUndoStack().push(new CSMWorld::AddNestedCommand(*(mModel->model()),
                                                                 mModel->getParentId(),
                                                                 row,
                                                                 mModel->getParentColumn()));
}

void CSVWorld::NestedTable::editCell()
{
    emit editRequest(mEditIdAction->getCurrentId(), "");
}
