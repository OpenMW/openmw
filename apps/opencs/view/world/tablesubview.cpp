
#include "tablesubview.hpp"

#include "table.hpp"

CSVWorld::TableSubView::TableSubView (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack,
    bool createAndDelete)
: SubView (id)
{
    setWidget (mTable = new Table (id, data, undoStack, createAndDelete));
}

void CSVWorld::TableSubView::setEditLock (bool locked)
{
    mTable->setEditLock (locked);
}