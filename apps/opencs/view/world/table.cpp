
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
#include "recordstatusdelegate.hpp"
#include "refidtypedelegate.hpp"
#include "util.hpp"

void CSVWorld::Table::contextMenuEvent (QContextMenuEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    QMenu menu (this);

    ///  \todo add menu items for select all and clear selection

    if (!mEditLock)
    {
        if (selectedRows.size()==1)
            menu.addAction (mEditAction);

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
    std::vector<std::string> revertableIds;

    if (mProxyModel->columnCount()>0)
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();

        for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
             ++iter)
        {
            QModelIndex index = mProxyModel->mapToSource (mProxyModel->index (iter->row(), 0));

            CSMWorld::RecordBase::State state =
                static_cast<CSMWorld::RecordBase::State> (
                mModel->data (mModel->index (index.row(), 1)).toInt());

            if (state!=CSMWorld::RecordBase::State_BaseOnly)
            {
                int columnIndex = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);

                std::string id = mModel->data (mModel->index (index.row(), columnIndex)).
                    toString().toUtf8().constData();

                revertableIds.push_back (id);
            }
        }
    }

    return revertableIds;
}

std::vector<std::string> CSVWorld::Table::listDeletableSelectedIds() const
{
    std::vector<std::string> deletableIds;

    if (mProxyModel->columnCount()>0)
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();

        for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
            ++iter)
        {
            QModelIndex index = mProxyModel->mapToSource (mProxyModel->index (iter->row(), 0));

            CSMWorld::RecordBase::State state =
                static_cast<CSMWorld::RecordBase::State> (
                mModel->data (mModel->index (index.row(), 1)).toInt());

            if (state!=CSMWorld::RecordBase::State_Deleted)
            {
                int columnIndex = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);

                std::string id = mModel->data (mModel->index (index.row(), columnIndex)).
                    toString().toUtf8().constData();

                deletableIds.push_back (id);
            }
        }
    }

    return deletableIds;
}

CSVWorld::Table::Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack,
    bool createAndDelete)
    : mUndoStack (undoStack), mCreateAction (0), mEditLock (false), mRecordStatusDisplay (0)
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
            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display> (
                mModel->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            CommandDelegate *delegate = CommandDelegateFactoryCollection::get().makeDelegate (display,
                undoStack, this);

            mDelegates.push_back (delegate);
            setItemDelegateForColumn (i, delegate);
        }
        else
            hideColumn (i);
    }

    mEditAction = new QAction (tr ("Edit Record"), this);
    connect (mEditAction, SIGNAL (triggered()), this, SLOT (editRecord()));
    addAction (mEditAction);

    if (createAndDelete)
    {
        mCreateAction = new QAction (tr ("Add Record"), this);
        connect (mCreateAction, SIGNAL (triggered()), this, SIGNAL (createRequest()));
        addAction (mCreateAction);
    }

    mRevertAction = new QAction (tr ("Revert Record"), this);
    connect (mRevertAction, SIGNAL (triggered()), this, SLOT (revertRecord()));
    addAction (mRevertAction);

    mDeleteAction = new QAction (tr ("Delete Record"), this);
    connect (mDeleteAction, SIGNAL (triggered()), this, SLOT (deleteRecord()));
    addAction (mDeleteAction);

    connect (mProxyModel, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (tableSizeUpdate()));

    /// \note This signal could instead be connected to a slot that filters out changes not affecting
    /// the records status column (for permanence reasons)
    connect (mProxyModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (tableSizeUpdate()));

    connect (selectionModel(), SIGNAL (selectionChanged (const QItemSelection&, const QItemSelection&)),
        this, SLOT (selectionSizeUpdate ()));
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

void CSVWorld::Table::editRecord()
{
    if (!mEditLock)
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();

        if (selectedRows.size()==1)
            emit editRequest (selectedRows.begin()->row());
    }
}

void CSVWorld::Table::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
    int columns = mModel->columnCount();

    for (int i=0; i<columns; ++i)
        if (QAbstractItemDelegate *delegate = itemDelegateForColumn (i))
            if (dynamic_cast<CommandDelegate&> (*delegate).
                updateEditorSetting (settingName, settingValue))
                emit dataChanged (mModel->index (0, i), mModel->index (mModel->rowCount()-1, i));
}

void CSVWorld::Table::tableSizeUpdate()
{
    int size = 0;
    int deleted = 0;
    int modified = 0;

    if (mModel->columnCount()>0)
    {
        int rows = mModel->rowCount();

        for (int i=0; i<rows; ++i)
        {
            QModelIndex index = mProxyModel->mapToSource (mProxyModel->index (i, 0));

            int columnIndex = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification);
            int state = mModel->data (mModel->index (index.row(), columnIndex)).toInt();

            switch (state)
            {
                case CSMWorld::RecordBase::State_BaseOnly: ++size; break;
                case CSMWorld::RecordBase::State_Modified: ++size; ++modified; break;
                case CSMWorld::RecordBase::State_ModifiedOnly: ++size; ++modified; break;
                case CSMWorld::RecordBase:: State_Deleted: ++deleted; ++modified; break;
            }
        }
    }

    tableSizeChanged (size, deleted, modified);
}

void CSVWorld::Table::selectionSizeUpdate()
{
    selectionSizeChanged (selectionModel()->selectedRows().size());
}

void CSVWorld::Table::requestFocus (const std::string& id)
{
    QModelIndex index = mProxyModel->getModelIndex (id, 0);

    if (index.isValid())
        scrollTo (index, QAbstractItemView::PositionAtTop);
}