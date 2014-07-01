#ifndef CSV_WORLD_NESTEDTABLE_H
#define CSV_WORLD_NESTEDTABLE_H

#include <QTableView>
#include <QtGui/qevent.h>

class QUndoStack;
class QAction;
class QContextMenuEvent;

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
        CSMWorld::NestedTableModel* mModel;
        
    public:
        NestedTable(QUndoStack& undoStack,
                    CSMWorld::NestedTableModel* model,
                    QWidget* parent = NULL);
        
    protected:
        void dragEnterEvent(QDragEnterEvent *event);
        
        void dragMoveEvent(QDragMoveEvent *event);
        
    private:
        void contextMenuEvent (QContextMenuEvent *event);
        
    private slots:
        void removeRowActionTriggered();
        
        void addNewRowActionTriggered();
        
    signals:
        void addNewRow();
        
        void removeRow(int row);
    };
}

#endif
