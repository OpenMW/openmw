#ifndef CSV_WORLD_TABLE_H
#define CSV_WORLD_TABLE_H

#include <vector>
#include <string>

#include <QTableView>

class QUndoStack;
class QAction;

namespace CSMWorld
{
    class Data;
    class UniversalId;
    class IdTableProxyModel;
    class IdTable;
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
            QAction *mRevertAction;
            QAction *mDeleteAction;
            CSMWorld::IdTableProxyModel *mProxyModel;
            CSMWorld::IdTable *mModel;
            bool mEditLock;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            std::vector<std::string> listRevertableSelectedIds() const;

            std::vector<std::string> listDeletableSelectedIds() const;

        public:

            Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack, bool createAndDelete);
            ///< \param createAndDelete Allow creation and deletion of records.

            void setEditLock (bool locked);

            CSMWorld::UniversalId getUniversalId (int row) const;

        private slots:

            void createRecord();

            void revertRecord();

            void deleteRecord();
    };
}

#endif
