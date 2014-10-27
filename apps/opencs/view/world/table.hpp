#ifndef CSV_WORLD_TABLE_H
#define CSV_WORLD_TABLE_H

#include <vector>
#include <string>

#include <QtGui/qevent.h>

#include "../../model/filter/node.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/universalid.hpp"
#include "dragrecordtable.hpp"

class QUndoStack;
class QAction;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class Data;
    class IdTableProxyModel;
    class IdTableBase;
    class CommandDispatcher;
}

namespace CSVWorld
{
    class CommandDelegate;

    ///< Table widget
    class Table : public DragRecordTable
    {
            Q_OBJECT

            enum DoubleClickAction
            {
                Action_None,
                Action_InPlaceEdit,
                Action_EditRecord,
                Action_View,
                Action_Revert,
                Action_Delete,
                Action_EditRecordAndClose,
                Action_ViewAndClose
            };

            std::vector<CommandDelegate *> mDelegates;
            QAction *mEditAction;
            QAction *mCreateAction;
            QAction *mCloneAction;
            QAction *mRevertAction;
            QAction *mDeleteAction;
            QAction *mMoveUpAction;
            QAction *mMoveDownAction;
            QAction *mViewAction;
            QAction *mEditCellAction;
            QAction *mPreviewAction;
            QAction *mExtendedDeleteAction;
            QAction *mExtendedRevertAction;
            CSMWorld::IdTableProxyModel *mProxyModel;
            CSMWorld::IdTableBase *mModel;
            int mRecordStatusDisplay;
            CSMWorld::CommandDispatcher *mDispatcher;
            CSMWorld::UniversalId mEditCellId;
            std::map<Qt::KeyboardModifiers, DoubleClickAction> mDoubleClickActions;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            void mouseMoveEvent(QMouseEvent *event);

            void dropEvent(QDropEvent *event);

        protected:

            virtual void mouseDoubleClickEvent (QMouseEvent *event);

        public:

            Table (const CSMWorld::UniversalId& id, bool createAndDelete,
                bool sorting, CSMDoc::Document& document);
            ///< \param createAndDelete Allow creation and deletion of records.
            /// \param sorting Allow changing order of rows in the view via column headers.

            virtual void setEditLock (bool locked);

            CSMWorld::UniversalId getUniversalId (int row) const;

            std::vector<std::string> getColumnsWithDisplay(CSMWorld::ColumnBase::Display display) const;

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const;

        signals:

            void editRequest (const CSMWorld::UniversalId& id, const std::string& hint);

            void selectionSizeChanged (int size);

            void tableSizeChanged (int size, int deleted, int modified);
            ///< \param size Number of not deleted records
            /// \param deleted Number of deleted records
            /// \param modified Number of added and modified records

            void createRequest();

            void cloneRequest(const CSMWorld::UniversalId&);

            void closeRequest();

        private slots:

            void editCell();

            void editRecord();

            void cloneRecord();

            void moveUpRecord();

            void moveDownRecord();

            void viewRecord();

            void previewRecord();

        public slots:

            void tableSizeUpdate();

            void selectionSizeUpdate();

            void requestFocus (const std::string& id);

            void recordFilterChanged (boost::shared_ptr<CSMFilter::Node> filter);

            void updateUserSetting (const QString &name, const QStringList &list);
    };
}

#endif
