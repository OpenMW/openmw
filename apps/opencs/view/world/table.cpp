
#include "table.hpp"

#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QUndoStack>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtableproxymodel.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/record.hpp"

namespace CSVWorld
{
    ///< \brief Getting the data out of an editor widget
    ///
    /// Really, Qt? Really?
    class NastyTableModelHack : public QAbstractTableModel
    {
            QAbstractItemModel& mModel;
            QVariant mData;

        public:

            NastyTableModelHack (QAbstractItemModel& model);

            int rowCount (const QModelIndex & parent = QModelIndex()) const;

            int columnCount (const QModelIndex & parent = QModelIndex()) const;

            QVariant data  (const QModelIndex & index, int role = Qt::DisplayRole) const;

            bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

            QVariant getData() const;
    };

    ///< \brief Use commands instead of manipulating the model directly
    class CommandDelegate : public QStyledItemDelegate
    {
            QUndoStack& mUndoStack;
            bool mEditLock;

        public:

            CommandDelegate (QUndoStack& undoStack, QObject *parent);

            void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const;

            void setEditLock (bool locked);
    };

}

CSVWorld::NastyTableModelHack::NastyTableModelHack (QAbstractItemModel& model)
: mModel (model)
{}

int CSVWorld::NastyTableModelHack::rowCount (const QModelIndex & parent) const
{
    return mModel.rowCount (parent);
}

int CSVWorld::NastyTableModelHack::columnCount (const QModelIndex & parent) const
{
    return mModel.columnCount (parent);
}

QVariant CSVWorld::NastyTableModelHack::data  (const QModelIndex & index, int role) const
{
    return mModel.data (index, role);
}

bool CSVWorld::NastyTableModelHack::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    mData = value;
    return true;
}

QVariant CSVWorld::NastyTableModelHack::getData() const
{
    return mData;
}

CSVWorld::CommandDelegate::CommandDelegate (QUndoStack& undoStack, QObject *parent)
: QStyledItemDelegate (parent), mUndoStack (undoStack), mEditLock (false)
{}

void CSVWorld::CommandDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
        const QModelIndex& index) const
{
    if (!mEditLock)
    {
        NastyTableModelHack hack (*model);
        QStyledItemDelegate::setModelData (editor, &hack, index);
        mUndoStack.push (new CSMWorld::ModifyCommand (*model, index, hack.getData()));
    }
    ///< \todo provide some kind of feedback to the user, indicating that editing is currently not possible.
}

void  CSVWorld::CommandDelegate::setEditLock (bool locked)
{
    mEditLock = locked;
}

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

    int columns = mModel->columnCount();

    for (int i=0; i<columns; ++i)
    {
        CommandDelegate *delegate = new CommandDelegate (undoStack, this);
        mDelegates.push_back (delegate);
        setItemDelegateForColumn (i, delegate);
    }

    mProxyModel = new CSMWorld::IdTableProxyModel (this);
    mProxyModel->setSourceModel (mModel);

    setModel (mProxyModel);
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    verticalHeader()->hide();
    setSortingEnabled (true);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

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