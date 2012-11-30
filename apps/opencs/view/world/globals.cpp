
#include "globals.hpp"

#include "table.hpp"

CSVWorld::Globals::Globals (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack)
: SubView (id)
{
    setWidget (mTable = new Table (id, data, undoStack));
}

void CSVWorld::Globals::setEditLock (bool locked)
{
    mTable->setEditLock (locked);
}