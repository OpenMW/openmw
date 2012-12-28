
#include "tablesubview.hpp"

#include "../../model/doc/document.hpp"

#include "table.hpp"

CSVWorld::TableSubView::TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete)
: SubView (id)
{
    setWidget (mTable = new Table (id, document.getData(), document.getUndoStack(), createAndDelete));
}

void CSVWorld::TableSubView::setEditLock (bool locked)
{
    mTable->setEditLock (locked);
}