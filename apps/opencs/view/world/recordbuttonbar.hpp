#ifndef CSV_WORLD_RECORDBUTTONBAR_H
#define CSV_WORLD_RECORDBUTTONBAR_H

#include <QWidget>

#include "../../model/world/universalid.hpp"

class QToolButton;
class QModelIndex;

namespace CSMWorld
{
    class IdTable;
    class CommandDispatcher;
}

namespace CSMPrefs
{
    class Setting;
}

namespace CSVWorld
{
    class TableBottomBox;

    /// \brief Button bar for use in dialogue-type subviews
    ///
    /// Contains the following buttons:
    /// - next/prev
    /// - clone
    /// - add
    /// - delete
    /// - revert
    /// - preview (optional)
    /// - view (optional)
    class RecordButtonBar : public QWidget
    {
            Q_OBJECT

            CSMWorld::UniversalId mId;
            CSMWorld::IdTable& mTable;
            TableBottomBox *mBottom;
            CSMWorld::CommandDispatcher *mCommandDispatcher;
            QToolButton *mPrevButton;
            QToolButton *mNextButton;
            QToolButton *mCloneButton;
            QToolButton *mAddButton;
            QToolButton *mDeleteButton;
            QToolButton *mRevertButton;
            bool mLocked;

        private:

            void updateModificationButtons();

            void updatePrevNextButtons();

        public:

            RecordButtonBar (const CSMWorld::UniversalId& id,
                CSMWorld::IdTable& table, TableBottomBox *bottomBox = nullptr,
                CSMWorld::CommandDispatcher *commandDispatcher = nullptr, QWidget *parent = nullptr);

            void setEditLock (bool locked);

        public slots:

            void universalIdChanged (const CSMWorld::UniversalId& id);

        private slots:

            void settingChanged (const CSMPrefs::Setting *setting);

            void cloneRequest();

            void nextId();

            void prevId();

            void rowNumberChanged (const QModelIndex& parent, int start, int end);

        signals:

            void showPreview();

            void viewRecord();

            void switchToRow (int row);
    };
}

#endif
