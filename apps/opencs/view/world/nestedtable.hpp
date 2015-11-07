#ifndef CSV_WORLD_NESTEDTABLE_H
#define CSV_WORLD_NESTEDTABLE_H

#include <QTableView>
#include <QtGui/qevent.h>

class QUndoStack;
class QAction;
class QContextMenuEvent;

namespace CSMWorld
{
    class NestedTableProxyModel;
    class UniversalId;
    class CommandDispatcher;
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
        CSMWorld::NestedTableProxyModel* mModel;
        CSMWorld::CommandDispatcher *mDispatcher;

    public:
        NestedTable(CSMDoc::Document& document,
                    CSMWorld::UniversalId id,
                    CSMWorld::NestedTableProxyModel* model,
                    QWidget* parent = NULL);

    protected:
        void dragEnterEvent(QDragEnterEvent *event);

        void dragMoveEvent(QDragMoveEvent *event);

    private:
        void contextMenuEvent (QContextMenuEvent *event);

    private slots:
        void removeRowActionTriggered();

        void addNewRowActionTriggered();
    };
}

#endif
