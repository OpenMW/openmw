
#include "tablesubview.hpp"

#include "../../model/doc/document.hpp"

#include "table.hpp"

CSVWorld::TableSubView::TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete)
: SubView (id)
{
    setWidget (mTable = new Table (id, document.getData(), document.getUndoStack(), createAndDelete));

    connect (mTable, SIGNAL (doubleClicked (const QModelIndex&)), this, SLOT (rowActivated (const QModelIndex&)));
}

void CSVWorld::TableSubView::setEditLock (bool locked)
{
    mTable->setEditLock (locked);
}

void CSVWorld::TableSubView::rowActivated (const QModelIndex& index)
{
    focusId (mTable->getUniversalId (index.row()));
}