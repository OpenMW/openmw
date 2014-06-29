#ifndef CSV_WORLD_NESTEDTABLE_H
#define CSV_WORLD_TABLE_H

#include <QTableView>
#include <QtGui/qevent.h>

class QUndoStack;
class QAction;

namespace CSMWorld
{
    class NestedTableModel;
}

namespace CSVWorld
{
    class NestedTable : public QTableView
    {
        Q_OBJECT

        std::vector<CommandDelegate*> mDelegates;
        QAction *mAddNewRowAction;
        QAction *mRemoveRowAction;
        CSMWorld::CommandDispatcher *mDispatcher;
        
    public:
        NestedTable(CSMDoc::Document& document, CSMWorld::NestedTableModel* model, const CSMWorld::UniversalId& id, QWidget* parent = NULL);
        
    protected:
        void dragEnterEvent(QDragEnterEvent *event);
        
        void dragMoveEvent(QDragMoveEvent *event);
    };
}
