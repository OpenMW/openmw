
#include "table.hpp"

#include <QHeaderView>
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QContextMenuEvent>
#include <QString>
#include <QtCore/qnamespace.h>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtableproxymodel.hpp"
#include "../../model/world/idtablebase.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/commanddispatcher.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"

void CSVWorld::Table::contextMenuEvent (QContextMenuEvent *event)
{
    // configure dispatcher
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    std::vector<std::string> records;

    int columnIndex = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
    {
        int row = mProxyModel->mapToSource (mProxyModel->index (iter->row(), 0)).row();

        records.push_back (mModel->data (
            mModel->index (row, columnIndex)).toString().toUtf8().constData());
    }

    mDispatcher->setSelection (records);

    std::vector<CSMWorld::UniversalId> extendedTypes = mDispatcher->getExtendedTypes();

    mDispatcher->setExtendedTypes (extendedTypes);

    // create context menu
    QMenu menu (this);

    ///  \todo add menu items for select all and clear selection

    {
        // Request UniversalId editing from table columns.

        int currRow = rowAt( event->y() ),
            currCol = columnAt( event->x() );

        currRow = mProxyModel->mapToSource(mProxyModel->index( currRow, 0 )).row();

        CSMWorld::ColumnBase::Display colDisplay =
            static_cast<CSMWorld::ColumnBase::Display>(
                mModel->headerData(
                    currCol,
                    Qt::Horizontal,
                    CSMWorld::ColumnBase::Role_Display ).toInt());

        QString cellData = mModel->data(mModel->index( currRow, currCol )).toString();
        CSMWorld::UniversalId::Type colType = CSMWorld::TableMimeData::convertEnums( colDisplay );

        if (    !cellData.isEmpty()
                && colType != CSMWorld::UniversalId::Type_None )
        {
            mEditCellAction->setText(tr("Edit '").append(cellData).append("'"));

            menu.addAction( mEditCellAction );

            mEditCellId = CSMWorld::UniversalId( colType, cellData.toUtf8().constData() );
        }
    }

    if (!mEditLock && !(mModel->getFeatures() & CSMWorld::IdTableBase::Feature_Constant))
    {
        if (selectedRows.size()==1)
        {
            menu.addAction (mEditAction);

            if (mCreateAction)
                menu.addAction(mCloneAction);
        }

        if (mCreateAction)
            menu.addAction (mCreateAction);

        if (mDispatcher->canRevert())
        {
            menu.addAction (mRevertAction);

            if (!extendedTypes.empty())
                menu.addAction (mExtendedRevertAction);
        }

        if (mDispatcher->canDelete())
        {
            menu.addAction (mDeleteAction);

            if (!extendedTypes.empty())
                menu.addAction (mExtendedDeleteAction);
        }

        if (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_ReorderWithinTopic)
        {
            /// \todo allow reordering of multiple rows
            if (selectedRows.size()==1)
            {
                int column = mModel->searchColumnIndex (CSMWorld::Columns::ColumnId_Topic);

                if (column==-1)
                    column = mModel->searchColumnIndex (CSMWorld::Columns::ColumnId_Journal);

                if (column!=-1)
                {
                    int row = mProxyModel->mapToSource (
                        mProxyModel->index (selectedRows.begin()->row(), 0)).row();

                    if (row>0 && mModel->data (mModel->index (row, column))==
                        mModel->data (mModel->index (row-1, column)))
                    {
                        menu.addAction (mMoveUpAction);
                    }

                    if (row<mModel->rowCount()-1 && mModel->data (mModel->index (row, column))==
                        mModel->data (mModel->index (row+1, column)))
                    {
                        menu.addAction (mMoveDownAction);
                    }
                }
            }
        }
    }

    if (selectedRows.size()==1)
    {
        int row = selectedRows.begin()->row();

        row = mProxyModel->mapToSource (mProxyModel->index (row, 0)).row();

        if (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_View)
        {
            CSMWorld::UniversalId id = mModel->view (row).first;

            int index = mDocument.getData().getCells().searchId (id.getId());
            // index==-1: the ID references a worldspace instead of a cell (ignore for now and go
            // ahead)

            if (index==-1 || !mDocument.getData().getCells().getRecord (index).isDeleted())
                menu.addAction (mViewAction);
        }

        if (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_Preview)
        {
            QModelIndex index = mModel->index (row,
                mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification));

            CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State> (
                mModel->data (index).toInt());

            if (state!=CSMWorld::RecordBase::State_Deleted)
                menu.addAction (mPreviewAction);
        }
    }

    menu.exec (event->globalPos());
}

void CSVWorld::Table::mouseDoubleClickEvent (QMouseEvent *event)
{
    Qt::KeyboardModifiers modifiers =
        event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

    QModelIndex index = currentIndex();

    selectionModel()->select (index,
        QItemSelectionModel::Clear | QItemSelectionModel::Select | QItemSelectionModel::Rows);

    std::map<Qt::KeyboardModifiers, DoubleClickAction>::iterator iter =
        mDoubleClickActions.find (modifiers);

    if (iter==mDoubleClickActions.end())
    {
        event->accept();
        return;
    }

    switch (iter->second)
    {
        case Action_None:

            event->accept();
            break;

        case Action_InPlaceEdit:

            DragRecordTable::mouseDoubleClickEvent (event);
            break;

        case Action_EditRecord:

            event->accept();
            editRecord();
            break;

        case Action_View:

            event->accept();
            viewRecord();
            break;

        case Action_Revert:

            event->accept();
            if (mDispatcher->canRevert())
                mDispatcher->executeRevert();
            break;

        case Action_Delete:

            event->accept();
            if (mDispatcher->canDelete())
                mDispatcher->executeDelete();
            break;

        case Action_EditRecordAndClose:

            event->accept();
            editRecord();
            emit closeRequest();
            break;

        case Action_ViewAndClose:

            event->accept();
            viewRecord();
            emit closeRequest();
            break;
    }
}

CSVWorld::Table::Table (const CSMWorld::UniversalId& id,
    bool createAndDelete, bool sorting, CSMDoc::Document& document)
: mCreateAction (0), mCloneAction(0), mRecordStatusDisplay (0),
  DragRecordTable(document)
{
    mModel = &dynamic_cast<CSMWorld::IdTableBase&> (*mDocument.getData().getTableModel (id));

    mProxyModel = new CSMWorld::IdTableProxyModel (this);
    mProxyModel->setSourceModel (mModel);

    mDispatcher = new CSMWorld::CommandDispatcher (document, id, this);

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
                mDispatcher, document, this);

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
    connect (mRevertAction, SIGNAL (triggered()), mDispatcher, SLOT (executeRevert()));
    addAction (mRevertAction);

    mDeleteAction = new QAction (tr ("Delete Record"), this);
    connect (mDeleteAction, SIGNAL (triggered()), mDispatcher, SLOT (executeDelete()));
    addAction (mDeleteAction);

    mMoveUpAction = new QAction (tr ("Move Up"), this);
    connect (mMoveUpAction, SIGNAL (triggered()), this, SLOT (moveUpRecord()));
    addAction (mMoveUpAction);

    mMoveDownAction = new QAction (tr ("Move Down"), this);
    connect (mMoveDownAction, SIGNAL (triggered()), this, SLOT (moveDownRecord()));
    addAction (mMoveDownAction);

    mEditCellAction = new QAction( tr("Edit Cell"), this );
    connect( mEditCellAction, SIGNAL(triggered()), this, SLOT(editCell()) );
    addAction( mEditCellAction );

    mViewAction = new QAction (tr ("View"), this);
    connect (mViewAction, SIGNAL (triggered()), this, SLOT (viewRecord()));
    addAction (mViewAction);

    mPreviewAction = new QAction (tr ("Preview"), this);
    connect (mPreviewAction, SIGNAL (triggered()), this, SLOT (previewRecord()));
    addAction (mPreviewAction);

    /// \todo add a user option, that redirects the extended action to an input panel (in
    /// the bottom bar) that lets the user select which record collections should be
    /// modified.

    mExtendedDeleteAction = new QAction (tr ("Extended Delete Record"), this);
    connect (mExtendedDeleteAction, SIGNAL (triggered()), mDispatcher, SLOT (executeExtendedDelete()));
    addAction (mExtendedDeleteAction);

    mExtendedRevertAction = new QAction (tr ("Extended Revert Record"), this);
    connect (mExtendedRevertAction, SIGNAL (triggered()), mDispatcher, SLOT (executeExtendedRevert()));
    addAction (mExtendedRevertAction);

    connect (mProxyModel, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (tableSizeUpdate()));

    /// \note This signal could instead be connected to a slot that filters out changes not affecting
    /// the records status column (for permanence reasons)
    connect (mProxyModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (tableSizeUpdate()));

    connect (selectionModel(), SIGNAL (selectionChanged (const QItemSelection&, const QItemSelection&)),
        this, SLOT (selectionSizeUpdate ()));

    setAcceptDrops(true);

    mDoubleClickActions.insert (std::make_pair (Qt::NoModifier, Action_InPlaceEdit));
    mDoubleClickActions.insert (std::make_pair (Qt::ShiftModifier, Action_EditRecord));
    mDoubleClickActions.insert (std::make_pair (Qt::ControlModifier, Action_View));
    mDoubleClickActions.insert (std::make_pair (Qt::ShiftModifier | Qt::ControlModifier, Action_EditRecordAndClose));
}

void CSVWorld::Table::setEditLock (bool locked)
{
    for (std::vector<CommandDelegate *>::iterator iter (mDelegates.begin()); iter!=mDelegates.end(); ++iter)
        (*iter)->setEditLock (locked);

    DragRecordTable::setEditLock(locked);
    mDispatcher->setEditLock (locked);
}

CSMWorld::UniversalId CSVWorld::Table::getUniversalId (int row) const
{
    row = mProxyModel->mapToSource (mProxyModel->index (row, 0)).row();

    int idColumn = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int typeColumn = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_RecordType);

    return CSMWorld::UniversalId (
        static_cast<CSMWorld::UniversalId::Type> (mModel->data (mModel->index (row, typeColumn)).toInt()),
        mModel->data (mModel->index (row, idColumn)).toString().toUtf8().constData());
}

void CSVWorld::Table::editRecord()
{
    if (!mEditLock || (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_Constant))
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();

        if (selectedRows.size()==1)
            emit editRequest (getUniversalId (selectedRows.begin()->row()), "");
    }
}

void CSVWorld::Table::cloneRecord()
{
    if (!mEditLock || (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_Constant))
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();
        const CSMWorld::UniversalId& toClone = getUniversalId(selectedRows.begin()->row());
        if (selectedRows.size()==1 && !mModel->isDeleted (toClone.getId()))
        {
            emit cloneRequest (toClone);
        }
    }
}

void CSVWorld::Table::moveUpRecord()
{
    if (mEditLock || (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_Constant))
        return;

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

            mDocument.getUndoStack().push (new CSMWorld::ReorderRowsCommand (
                dynamic_cast<CSMWorld::IdTable&> (*mModel), row, newOrder));
        }
    }
}

void CSVWorld::Table::moveDownRecord()
{
    if (mEditLock || (mModel->getFeatures() & CSMWorld::IdTableBase::Feature_Constant))
        return;

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

            mDocument.getUndoStack().push (new CSMWorld::ReorderRowsCommand (
                dynamic_cast<CSMWorld::IdTable&> (*mModel), row, newOrder));
        }
    }
}

void CSVWorld::Table::editCell()
{
    emit editRequest( mEditCellId, std::string() );
}

void CSVWorld::Table::viewRecord()
{
    if (!(mModel->getFeatures() & CSMWorld::IdTableBase::Feature_View))
        return;

    QModelIndexList selectedRows = selectionModel()->selectedRows();

    if (selectedRows.size()==1)
    {
        int row = selectedRows.begin()->row();

        row = mProxyModel->mapToSource (mProxyModel->index (row, 0)).row();

        std::pair<CSMWorld::UniversalId, std::string> params = mModel->view (row);

        if (params.first.getType()!=CSMWorld::UniversalId::Type_None)
            emit editRequest (params.first, params.second);
    }
}

void CSVWorld::Table::previewRecord()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    if (selectedRows.size()==1)
    {
        std::string id = getUniversalId (selectedRows.begin()->row()).getId();

        QModelIndex index = mModel->getModelIndex (id,
            mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification));

        if (mModel->data (index)!=CSMWorld::RecordBase::State_Deleted)
            emit editRequest (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Preview, id),
                "");
    }
}

void CSVWorld::Table::updateUserSetting
                                (const QString &name, const QStringList &list)
{
    if (name=="records/type-format" || name=="records/status-format")
    {
        int columns = mModel->columnCount();

        for (int i=0; i<columns; ++i)
            if (QAbstractItemDelegate *delegate = itemDelegateForColumn (i))
            {
                dynamic_cast<CommandDelegate&>
                                        (*delegate).updateUserSetting (name, list);
                {
                    emit dataChanged (mModel->index (0, i),
                                    mModel->index (mModel->rowCount()-1, i));
                }
            }
        return;
    }

    QString base ("table-input/double");
    if (name.startsWith (base))
    {
        QString modifierString = name.mid (base.size());
        Qt::KeyboardModifiers modifiers = 0;

        if (modifierString=="-s")
            modifiers = Qt::ShiftModifier;
        else if (modifierString=="-c")
            modifiers = Qt::ControlModifier;
        else if (modifierString=="-sc")
            modifiers = Qt::ShiftModifier | Qt::ControlModifier;

        DoubleClickAction action = Action_None;

        QString value = list.at (0);

        if (value=="Edit in Place")
            action = Action_InPlaceEdit;
        else if (value=="Edit Record")
            action = Action_EditRecord;
        else if (value=="View")
            action = Action_View;
        else if (value=="Revert")
            action = Action_Revert;
        else if (value=="Delete")
            action = Action_Delete;
        else if (value=="Edit Record and Close")
            action = Action_EditRecordAndClose;
        else if (value=="View and Close")
            action = Action_ViewAndClose;

        mDoubleClickActions[modifiers] = action;

        return;
    }
}

void CSVWorld::Table::tableSizeUpdate()
{
    int size = 0;
    int deleted = 0;
    int modified = 0;

    if (mProxyModel->columnCount()>0)
    {
        int rows = mProxyModel->rowCount();

        int columnIndex = mModel->searchColumnIndex (CSMWorld::Columns::ColumnId_Modification);

        if (columnIndex!=-1)
        {
            for (int i=0; i<rows; ++i)
            {
                QModelIndex index = mProxyModel->mapToSource (mProxyModel->index (i, 0));

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
        else
            size = rows;
    }

    emit tableSizeChanged (size, deleted, modified);
}

void CSVWorld::Table::selectionSizeUpdate()
{
    emit selectionSizeChanged (selectionModel()->selectedRows().size());
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
    tableSizeUpdate();
    selectionSizeUpdate();
}

void CSVWorld::Table::mouseMoveEvent (QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        startDragFromTable(*this);
    }
}

void CSVWorld::Table::dropEvent(QDropEvent *event)
{
    QModelIndex index = indexAt (event->pos());

    if (!index.isValid())
    {
        return;
    }

    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument (mDocument))
    {
        CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                                                (mModel->headerData (index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

        if (mime->holdsType (display))
        {
            CSMWorld::UniversalId record (mime->returnMatching (display));

            std::auto_ptr<CSMWorld::ModifyCommand> command (new CSMWorld::ModifyCommand
                    (*mProxyModel, index, QVariant (QString::fromUtf8 (record.getId().c_str()))));

            mDocument.getUndoStack().push (command.release());
        }
    } //TODO handle drops from different document
}

std::vector<std::string> CSVWorld::Table::getColumnsWithDisplay(CSMWorld::ColumnBase::Display display) const
{
    const int count = mModel->columnCount();

    std::vector<std::string> titles;
    for (int i = 0; i < count; ++i)
    {
        CSMWorld::ColumnBase::Display columndisplay = static_cast<CSMWorld::ColumnBase::Display>
                                                     (mModel->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

        if (display == columndisplay)
        {
            titles.push_back(mModel->headerData (i, Qt::Horizontal).toString().toUtf8().constData());
        }
    }
    return titles;
}

std::vector< CSMWorld::UniversalId > CSVWorld::Table::getDraggedRecords() const
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();
    std::vector<CSMWorld::UniversalId> idToDrag;

    foreach (QModelIndex it, selectedRows) //I had a dream. Dream where you could use C++11 in OpenMW.
    {
        idToDrag.push_back (getUniversalId (it.row()));
    }

    return idToDrag;
}

