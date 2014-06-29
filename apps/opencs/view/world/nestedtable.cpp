#include "nestedtable.hpp"

NestedTable::NestedTable(CSMDoc::Document& document, CSMWorld::NestedTableModel* model, const CSMWorld::UniversalId& id, QWidget* parent)
    : QTableView(parent)
{
    QTableView::setModel(model);
    setAcceptDrops(true);

    int columns = mModel->columnCount();

    for(int i = 0 ; i < columns; ++i)
    {
        CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display> (
            mModel->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

        CommandDelegate *delegate = CommandDelegateFactoryCollection::get().makeDelegate(display,
                                                                                         document.getUndoStack(),
                                                                                         this);
    }
}
