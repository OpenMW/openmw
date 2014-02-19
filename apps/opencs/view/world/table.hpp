#ifndef CSV_WORLD_TABLE_H
#define CSV_WORLD_TABLE_H

#include <vector>
#include <string>

#include <QTableView>
#include <QtGui/qevent.h>

#include "../../model/filter/node.hpp"

namespace CSMDoc {
    class Document;
}

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
            QAction *mEditAction;
            QAction *mCreateAction;
            QAction *mCloneAction;
            QAction *mRevertAction;
            QAction *mDeleteAction;
            QAction *mMoveUpAction;
            QAction *mMoveDownAction;
            CSMWorld::IdTableProxyModel *mProxyModel;
            CSMWorld::IdTable *mModel;
            bool mEditLock;
            int mRecordStatusDisplay;

            /// \brief This variable is used exclusivly for checking if dropEvents came from the same document. Most likely you
            /// should NOT use it for anything else.
            const CSMDoc::Document& mDocument;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            std::vector<std::string> listRevertableSelectedIds() const;

            std::vector<std::string> listDeletableSelectedIds() const;

            void mouseMoveEvent(QMouseEvent *event);

            void dragEnterEvent(QDragEnterEvent *event);

            void dropEvent(QDropEvent *event);

            void dragMoveEvent(QDragMoveEvent *event);


        public:

            Table (const CSMWorld::UniversalId& id, CSMWorld::Data& data, QUndoStack& undoStack, bool createAndDelete,
                   bool sorting, const CSMDoc::Document& document);

            ///< \param createAndDelete Allow creation and deletion of records.
            /// \param sorting Allow changing order of rows in the view via column headers.

            void setEditLock (bool locked);

            CSMWorld::UniversalId getUniversalId (int row) const;

            void updateEditorSetting (const QString &settingName, const QString &settingValue);

        signals:

            void editRequest (int row);

            void selectionSizeChanged (int size);

            void tableSizeChanged (int size, int deleted, int modified);
            ///< \param size Number of not deleted records
            /// \param deleted Number of deleted records
            /// \param modified Number of added and modified records

            void createRequest();
            void cloneRequest(const CSMWorld::UniversalId&);

        private slots:

            void revertRecord();

            void deleteRecord();

            void editRecord();

            void cloneRecord();

            void moveUpRecord();

            void moveDownRecord();

        public slots:

            void tableSizeUpdate();

            void selectionSizeUpdate();

            void requestFocus (const std::string& id);

            void recordFilterChanged (boost::shared_ptr<CSMFilter::Node> filter);
    };
}

#endif
