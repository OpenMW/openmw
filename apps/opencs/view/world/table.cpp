
#include "table.hpp"

#include <QHeaderView>

#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtableproxymodel.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/record.hpp"

#include "util.hpp"

void CSVWorld::Table::contextMenuEvent (QContextMenuEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    QMenu menu (this);

    ///  \todo add menu items for select all and clear selection

    if (!mEditLock)
    {
        if (mCreateAction)
            menu.addAction (mCreateAction);

        if (listRevertableSelectedIds().size()>0)
            menu.addAction (mRevertAction);

        if (listDeletableSelectedIds().size()>0)
            menu.addAction (mDeleteAction);
    }

    menu.exec (event->globalPos());
}

std::vector<std::string> CSVWorld::Table::listRevertableSelectedIds() const
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    std::vector<std::string> revertableIds;

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end(); ++iter)
    {
        std::string id = mProxyModel->data (*iter).toString().toStdString();

        CSMWorld::RecordBase::State state =
            static_cast<CSMWorld::RecordBase::State> (mModel->data (mModel->getModelIndex (id, 1)).toInt());

        if (state!=CSMWorld::RecordBase::State_BaseOnly)
            revertableIds.push_back (id);
    }

    return revertableIds;
}

std::vector<std::string> CSVWorld::Table::listDeletableSelectedIds() const
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    std::vector<std::string> deletableIds;

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end(); ++iter)
    {
        std::string id = mProxyModel->data (*iter).toString().toStdString();

        CSMWorld::RecordBase::State state =
            static_cast<CSMWorld::RecordBase::State> (mModel->data (mModel->getModelIndex (id, 1)).toInt());

        if (state!=CSMWorld::RecordBase::State_Deleted)
            deletableIds.push_back (id);
    }

    return deletableIds;
}

CSVWorld::Table::Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack,
    bool createAndDelete)
: mUndoStack (undoStack), mCreateAction (0), mEditLock (false)
{
    mModel = &dynamic_cast<CSMWorld::IdTable&> (*data.getTableModel (id));

    mProxyModel = new CSMWorld::IdTableProxyModel (this);
    mProxyModel->setSourceModel (mModel);

    setModel (mProxyModel);
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    verticalHeader()->hide();
    setSortingEnabled (true);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    int columns = mModel->columnCount();

    for (int i=0; i<columns; ++i)
    {
        int flags = mModel->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Table)
        {
            CommandDelegate *delegate = new CommandDelegate (undoStack, this);
            mDelegates.push_back (delegate);
            setItemDelegateForColumn (i, delegate);
        }
        else
            hideColumn (i);
    }

    /// \todo make initial layout fill the whole width of the table

    if (createAndDelete)
    {
        mCreateAction = new QAction (tr ("Add Record"), this);
        connect (mCreateAction, SIGNAL (triggered()), this, SLOT (createRecord()));
        addAction (mCreateAction);
    }

    mRevertAction = new QAction (tr ("Revert Record"), this);
    connect (mRevertAction, SIGNAL (triggered()), this, SLOT (revertRecord()));
    addAction (mRevertAction);

    mDeleteAction = new QAction (tr ("Delete Record"), this);
    connect (mDeleteAction, SIGNAL (triggered()), this, SLOT (deleteRecord()));
    addAction (mDeleteAction);
}

void CSVWorld::Table::setEditLock (bool locked)
{
    for (std::vector<CommandDelegate *>::iterator iter (mDelegates.begin()); iter!=mDelegates.end(); ++iter)
        (*iter)->setEditLock (locked);

    mEditLock = locked;
}

CSMWorld::UniversalId CSVWorld::Table::getUniversalId (int row) const
{
    return CSMWorld::UniversalId (
        static_cast<CSMWorld::UniversalId::Type> (mProxyModel->data (mProxyModel->index (row, 2)).toInt()),
        mProxyModel->data (mProxyModel->index (row, 0)).toString().toStdString());
}

#include <sstream> /// \todo remove
void CSVWorld::Table::createRecord()
{
    if (!mEditLock)
    {
        /// \todo ask the user for an ID instead.
        static int index = 0;

        std::ostringstream stream;
        stream << "id" << index++;

        mUndoStack.push (new CSMWorld::CreateCommand (*mProxyModel, stream.str()));
    }
}

void CSVWorld::Table::revertRecord()
{
    if (!mEditLock)
    {
        std::vector<std::string> revertableIds = listRevertableSelectedIds();

        if (revertableIds.size()>0)
        {
            if (revertableIds.size()>1)
                mUndoStack.beginMacro (tr ("Revert multiple records"));

            for (std::vector<std::string>::const_iterator iter (revertableIds.begin()); iter!=revertableIds.end(); ++iter)
                mUndoStack.push (new CSMWorld::RevertCommand (*mModel, *iter));

            if (revertableIds.size()>1)
                mUndoStack.endMacro();
        }
    }
}

void CSVWorld::Table::deleteRecord()
{
    if (!mEditLock)
    {
        std::vector<std::string> deletableIds = listDeletableSelectedIds();

        if (deletableIds.size()>0)
        {
            if (deletableIds.size()>1)
                mUndoStack.beginMacro (tr ("Delete multiple records"));

            for (std::vector<std::string>::const_iterator iter (deletableIds.begin()); iter!=deletableIds.end(); ++iter)
                mUndoStack.push (new CSMWorld::DeleteCommand (*mModel, *iter));

            if (deletableIds.size()>1)
                mUndoStack.endMacro();
        }
    }
}