#ifndef CSV_WORLD_GLOBALS_H
#define CSV_WORLD_GLOBALS_H

#include "subview.hpp"

class QUndoStack;

namespace CSVWorld
{
    class Table;

    class Globals : public SubView
    {
            Table *mTable;

        public:

            Globals (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack);

            virtual void setEditLock (bool locked);
    };
}

#endif