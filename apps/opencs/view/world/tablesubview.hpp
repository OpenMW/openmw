#ifndef CSV_WORLD_TABLESUBVIEW_H
#define CSV_WORLD_TABLESUBVIEW_H

#include "subview.hpp"

class QUndoStack;

namespace CSVWorld
{
    class Table;

    class TableSubView : public SubView
    {
            Table *mTable;

        public:

            TableSubView (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack,
                bool createAndDelete);

            virtual void setEditLock (bool locked);
    };
}

#endif