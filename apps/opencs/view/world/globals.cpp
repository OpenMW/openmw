
#include "globals.hpp"

#include "table.hpp"

CSVWorld::Globals::Globals (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack)
: SubView (id)
{
    QTableView *table = new Table (id, data, undoStack);

    setWidget (table);
}