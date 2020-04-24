#ifndef CSV_WORLD_TABLE_H
#define CSV_WORLD_TABLE_H

#include <vector>
#include <string>

#include <QEvent>

#include "../../model/filter/node.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/universalid.hpp"
#include "dragrecordtable.hpp"

class QAction;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class IdTableProxyModel;
    class IdTableBase;
    class CommandDispatcher;
}

namespace CSMPrefs
{
    class Setting;
}

namespace CSVWorld
{
    class CommandDelegate;
    class TableEditIdAction;

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
            QAction *mTouchAction;
            QAction *mRevertAction;
            QAction *mDeleteAction;
            QAction *mMoveUpAction;
            QAction *mMoveDownAction;
            QAction *mViewAction;
            QAction *mPreviewAction;
            QAction *mExtendedDeleteAction;
            QAction *mExtendedRevertAction;
            QAction *mHelpAction;
            TableEditIdAction *mEditIdAction;
            CSMWorld::IdTableProxyModel *mProxyModel;
            CSMWorld::IdTableBase *mModel;
            int mRecordStatusDisplay;
            CSMWorld::CommandDispatcher *mDispatcher;
            std::map<Qt::KeyboardModifiers, DoubleClickAction> mDoubleClickActions;
            bool mJumpToAddedRecord;
            bool mUnselectAfterJump;

        private:

            void contextMenuEvent (QContextMenuEvent *event);

            void mouseMoveEvent(QMouseEvent *event);

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

            std::vector<std::string> getSelectedIds() const;

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

            void touchRequest(const std::vector<CSMWorld::UniversalId>& ids);

            void closeRequest();

            void extendedDeleteConfigRequest(const std::vector<std::string> &selectedIds);

            void extendedRevertConfigRequest(const std::vector<std::string> &selectedIds);

        private slots:

            void editCell();

            static void openHelp();

            void editRecord();

            void cloneRecord();

            void touchRecord();

            void moveUpRecord();

            void moveDownRecord();

            void viewRecord();

            void previewRecord();

            void executeExtendedDelete();

            void executeExtendedRevert();

        public slots:

            void settingChanged (const CSMPrefs::Setting *setting);

            void tableSizeUpdate();

            void selectionSizeUpdate();

            void requestFocus (const std::string& id);

            void recordFilterChanged (std::shared_ptr<CSMFilter::Node> filter);

            void rowAdded(const std::string &id);
    };
}

#endif
