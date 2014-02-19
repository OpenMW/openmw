
#include "table.hpp"

#include <QHeaderView>

#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>
#include <QString>
#include <QtCore/qnamespace.h>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtableproxymodel.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"

void CSVWorld::Table::contextMenuEvent (QContextMenuEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    QMenu menu (this);

    ///  \todo add menu items for select all and clear selection

    if (!mEditLock)
    {
        if (selectedRows.size()==1)
        {
            menu.addAction (mEditAction);
            if (mCreateAction)
                menu.addAction(mCloneAction);
        }

        if (mCreateAction)
            menu.addAction (mCreateAction);

        /// \todo Reverting temporarily disabled on tables that support reordering, because
        /// revert logic currently can not handle reordering.
        if (mModel->getReordering()==CSMWorld::IdTable::Reordering_None)
            if (listRevertableSelectedIds().size()>0)
                menu.addAction (mRevertAction);

        if (listDeletableSelectedIds().size()>0)
            menu.addAction (mDeleteAction);

        if (mModel->getReordering()==CSMWorld::IdTable::Reordering_WithinTopic)
        {
            /// \todo allow reordering of multiple rows
            if (selectedRows.size()==1)
            {
                int row =selectedRows.begin()->row();

                int column = mModel->searchColumnIndex (CSMWorld::Columns::ColumnId_Topic);

                if (column==-1)
                    column = mModel->searchColumnIndex (CSMWorld::Columns::ColumnId_Journal);

                if (column!=-1)
                {
                    if (row>0 && mProxyModel->data (mProxyModel->index (row, column))==
                        mProxyModel->data (mProxyModel->index (row-1, column)))
                    {
                        menu.addAction (mMoveUpAction);
                    }

                    if (row<mProxyModel->rowCount()-1 && mProxyModel->data (mProxyModel->index (row, column))==
                        mProxyModel->data (mProxyModel->index (row+1, column)))
                    {
                        menu.addAction (mMoveDownAction);
                    }
                }
            }
        }
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

            // check record state
            CSMWorld::RecordBase::State state =
                static_cast<CSMWorld::RecordBase::State> (
                mModel->data (mModel->index (index.row(), 1)).toInt());

            if (state==CSMWorld::RecordBase::State_Deleted)
                continue;

            // check other columns (only relevant for a subset of the tables)
            int dialogueTypeIndex =
                mModel->searchColumnIndex (CSMWorld::Columns::ColumnId_DialogueType);

            if (dialogueTypeIndex!=-1)
            {
                int type = mModel->data (mModel->index (index.row(), dialogueTypeIndex)).toInt();

                if (type!=ESM::Dialogue::Topic && type!=ESM::Dialogue::Journal)
                    continue;
            }

            // add the id to the collection
            int columnIndex = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);

            std::string id = mModel->data (mModel->index (index.row(), columnIndex)).
                toString().toUtf8().constData();

            deletableIds.push_back (id);
        }
    }

    return deletableIds;
}

CSVWorld::Table::Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack,
    bool createAndDelete, bool sorting, const CSMDoc::Document& document)
    : mUndoStack (undoStack), mCreateAction (0), mCloneAction(0), mEditLock (false), mRecordStatusDisplay (0), mDocument(document)
{
    mModel = &dynamic_cast<CSMWorld::IdTable&> (*data.getTableModel (id));

    mProxyModel = new CSMWorld::IdTableProxyModel (this);
    mProxyModel->setSourceModel (mModel);

    setModel (mProxyModel);
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    verticalHeader()->hide();
    setSortingEnabled (sorting);
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

        mCloneAction = new QAction (tr ("Clone Record"), this);
        connect(mCloneAction, SIGNAL (triggered()), this, SLOT (cloneRecord()));
        addAction(mCloneAction);
    }

    mRevertAction = new QAction (tr ("Revert Record"), this);
    connect (mRevertAction, SIGNAL (triggered()), this, SLOT (revertRecord()));
    addAction (mRevertAction);

    mDeleteAction = new QAction (tr ("Delete Record"), this);
    connect (mDeleteAction, SIGNAL (triggered()), this, SLOT (deleteRecord()));
    addAction (mDeleteAction);

    mMoveUpAction = new QAction (tr ("Move Up"), this);
    connect (mMoveUpAction, SIGNAL (triggered()), this, SLOT (moveUpRecord()));
    addAction (mMoveUpAction);

    mMoveDownAction = new QAction (tr ("Move Down"), this);
    connect (mMoveDownAction, SIGNAL (triggered()), this, SLOT (moveDownRecord()));
    addAction (mMoveDownAction);

    connect (mProxyModel, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (tableSizeUpdate()));

    /// \note This signal could instead be connected to a slot that filters out changes not affecting
    /// the records status column (for permanence reasons)
    connect (mProxyModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (tableSizeUpdate()));

    connect (selectionModel(), SIGNAL (selectionChanged (const QItemSelection&, const QItemSelection&)),
        this, SLOT (selectionSizeUpdate ()));

    setAcceptDrops(true);
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

void CSVWorld::Table::cloneRecord()
{
    if (!mEditLock)
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();
        const CSMWorld::UniversalId& toClone = getUniversalId(selectedRows.begin()->row());
        if (selectedRows.size()==1 && !mModel->getRecord(toClone.getId()).isDeleted())
        {
            emit cloneRequest (toClone);
        }
    }
}

void CSVWorld::Table::moveUpRecord()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    if (selectedRows.size()==1)
    {
        int row2 =selectedRows.begin()->row();

        if (row2>0)
        {
            int row = row2-1;

            row = mProxyModel->mapToSource (mProxyModel->index (row, 0)).row();
            row2 = mProxyModel->mapToSource (mProxyModel->index (row2, 0)).row();

            if (row2<=row)
                throw std::runtime_error ("Inconsistent row order");

            std::vector<int> newOrder (row2-row+1);
            newOrder[0] = row2-row;
            newOrder[row2-row] = 0;
            for (int i=1; i<row2-row; ++i)
                newOrder[i] = i;

            mUndoStack.push (new CSMWorld::ReorderRowsCommand (*mModel, row, newOrder));
        }
    }
}

void CSVWorld::Table::moveDownRecord()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    if (selectedRows.size()==1)
    {
        int row =selectedRows.begin()->row();

        if (row<mProxyModel->rowCount()-1)
        {
            int row2 = row+1;

            row = mProxyModel->mapToSource (mProxyModel->index (row, 0)).row();
            row2 = mProxyModel->mapToSource (mProxyModel->index (row2, 0)).row();

            if (row2<=row)
                throw std::runtime_error ("Inconsistent row order");

            std::vector<int> newOrder (row2-row+1);
            newOrder[0] = row2-row;
            newOrder[row2-row] = 0;
            for (int i=1; i<row2-row; ++i)
                newOrder[i] = i;

            mUndoStack.push (new CSMWorld::ReorderRowsCommand (*mModel, row, newOrder));
        }
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

    if (mProxyModel->columnCount()>0)
    {
        int rows = mProxyModel->rowCount();

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

void CSVWorld::Table::recordFilterChanged (boost::shared_ptr<CSMFilter::Node> filter)
{
    mProxyModel->setFilter (filter);
}

void CSVWorld::Table::mouseMoveEvent (QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();

        if (selectedRows.size() == 0)
        {
            return;
        }

        QDrag* drag = new QDrag (this);
        CSMWorld::TableMimeData* mime = NULL;

        if (selectedRows.size() == 1)
        {
            mime = new CSMWorld::TableMimeData (getUniversalId (selectedRows.begin()->row()), mDocument);
        }
        else
        {
            std::vector<CSMWorld::UniversalId> idToDrag;

            foreach (QModelIndex it, selectedRows) //I had a dream. Dream where you could use C++11 in OpenMW.
            {
                idToDrag.push_back (getUniversalId (it.row()));
            }

            mime = new CSMWorld::TableMimeData (idToDrag, mDocument);
        }

        drag->setMimeData (mime);
        drag->setPixmap (QString::fromStdString (mime->getIcon()));
        drag->exec();
    }

}

void CSVWorld::Table::dragEnterEvent(QDragEnterEvent *event)
{
        event->acceptProposedAction();
}

void CSVWorld::Table::dropEvent(QDropEvent *event)
{
    QModelIndex index = indexAt (event->pos());

    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (mime->fromDocument (mDocument))
    {
        CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                                                (mModel->headerData (index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

        if (mime->holdsType (display))
        {
            CSMWorld::UniversalId record (mime->returnMatching (display));

            std::auto_ptr<CSMWorld::ModifyCommand> command (new CSMWorld::ModifyCommand
                    (*mProxyModel, index, QVariant (QString::fromStdString (record.getId()))));

            mUndoStack.push (command.release());
        }
    } //TODO handle drops from different document
}

void CSVWorld::Table::dragMoveEvent(QDragMoveEvent *event)
{
        event->accept();
}
