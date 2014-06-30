#ifndef CSV_WORLD_NESTEDTABLE_H
#define CSV_WORLD_NESTEDTABLE_H

#include <QTableView>
#include <QtGui/qevent.h>

class QUndoStack;
class QAction;

namespace CSMWorld
{
    class NestedTableModel;
    class UniversalId;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class NestedTable : public QTableView
    {
        Q_OBJECT

        QAction *mAddNewRowAction;
        QAction *mRemoveRowAction;
        QUndoStack& mUndoStack;
        
    public:
        NestedTable(QUndoStack& undoStack,
                    CSMWorld::NestedTableModel* model,
                    QWidget* parent = NULL);
        
    protected:
        void dragEnterEvent(QDragEnterEvent *event);
        
        void dragMoveEvent(QDragMoveEvent *event);
    };
}

#endif
