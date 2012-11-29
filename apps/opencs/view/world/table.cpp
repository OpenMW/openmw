
#include "table.hpp"

#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QUndoStack>

#include "../../model/world/data.hpp"

#include "../../model/world/commands.hpp"

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

        public:

            CommandDelegate (QUndoStack& undoStack, QObject *parent);

            void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const;
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
: QStyledItemDelegate (parent), mUndoStack (undoStack)
{}

void CSVWorld::CommandDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
        const QModelIndex& index) const
{
    NastyTableModelHack hack (*model);
    QStyledItemDelegate::setModelData (editor, &hack, index);
    mUndoStack.push (new CSMWorld::ModifyCommand (*model, index, hack.getData()));
}


CSVWorld::Table::Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack)
{
    QAbstractTableModel *model = data.getTableModel (id);

    int columns = model->columnCount();

    for (int i=0; i<columns; ++i)
    {
        CommandDelegate *delegate = new CommandDelegate (undoStack, this);
        setItemDelegateForColumn (i, delegate);
    }

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel (this);
    proxyModel->setSourceModel (model);

    setModel (proxyModel);
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    verticalHeader()->hide();
    setSortingEnabled (true);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    /// \todo make initial layout fill the whole width of the table
}