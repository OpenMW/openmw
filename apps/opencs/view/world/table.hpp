#ifndef CSV_WORLD_TABLE_H
#define CSV_WORLD_TABLE_H

#include <QTableView>

class QUndoStack;

namespace CSMWorld
{
    class Data;
    class UniversalId;
}

namespace CSVWorld
{
    ///< Table widget
    class Table : public QTableView
    {
        public:

            Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack);
    };
}

#endif
