#ifndef CSV_WORLD_RECORDBUTTONBAR_H
#define CSV_WORLD_RECORDBUTTONBAR_H

#include <QWidget>

namespace CSMWorld
{
    class IdTable;
    class CommandDispatcher;
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

            CSMWorld::IdTable& mTable;
            TableBottomBox *mBottom;
            CSMWorld::CommandDispatcher *mCommandDispatcher;

        public:

            RecordButtonBar (CSMWorld::IdTable& table, TableBottomBox *bottomBox = 0,
                CSMWorld::CommandDispatcher *commandDispatcher = 0, QWidget *parent = 0);

        signals:

            void showPreview();

            void viewRecord();

            void nextId();

            void prevId();

            void cloneRequest();

    };
}

#endif
