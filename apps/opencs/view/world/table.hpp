#ifndef CSV_WORLD_TABLE_H
#define CSV_WORLD_TABLE_H

#include <vector>

#include <QTableView>

class QUndoStack;
class QAction;

namespace CSMWorld
{
    class Data;
    class UniversalId;
    class IdTableProxyModel;
}

namespace CSVWorld
{
    class CommandDelegate;

    ///< Table widget
    class Table : public QTableView
    {
            Q_OBJECT

            std::vector<CommandDelegate *> mDelegates;
            QUndoStack& mUndoStack;
            QAction *mCreateAction;
            CSMWorld::IdTableProxyModel *mModel;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

        public:

            Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack, bool createAndDelete);
            ///< \param createAndDelete Allow creation and deletion of records.

            void setEditLock (bool locked);

        private slots:

            void createRecord();
    };
}

#endif
