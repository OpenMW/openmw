
#include "globals.hpp"

#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QUndoStack>

#include "../../model/world/data.hpp"

#include "../../model/world/commands.hpp"

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

CSVWorld::Globals::Globals (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack)
: SubView (id)
{
    QTableView *table = new QTableView();

    setWidget (table);

    QAbstractTableModel *model = data.getTableModel (id);

    int columns = model->columnCount();

    for (int i=0; i<columns; ++i)
    {
        CommandDelegate *delegate = new CommandDelegate (undoStack, table);
        table->setItemDelegateForColumn (i, delegate);
    }

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel (this);
    proxyModel->setSourceModel (model);

    table->setModel (proxyModel);
    table->horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    table->verticalHeader()->hide();
    table->setSortingEnabled (true);
    table->setSelectionBehavior (QAbstractItemView::SelectRows);
    table->setSelectionMode (QAbstractItemView::ExtendedSelection);

    /// \todo make initial layout fill the whole width of the table
}