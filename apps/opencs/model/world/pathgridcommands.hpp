#ifndef CSM_WOLRD_PATHGRIDCOMMANDS_H
#define CSM_WOLRD_PATHGRIDCOMMANDS_H

#include <QObject>

#include "commands.hpp"

namespace CSVRender
{
    class Cell;
}

namespace CSMWorld
{
    class IdTree;
    class NestedTableWrapperBase;

    class ModifyPathgridCommand : public QObject, public QUndoCommand, private NestedTableStoring
    {
        Q_OBJECT

            IdTree& mModel;
            std::string mId;

            int mParentColumn;

            NestedTableWrapperBase* mRecord;

        public:

            ModifyPathgridCommand(IdTree& model,
                    const std::string& id, int parentColumn, NestedTableWrapperBase* newRecord,
                    QUndoCommand* parent = 0);

            virtual void redo();

            virtual void undo();

        signals:

            void undoActioned();
    };

    class SignalHandler : public QObject
    {
        Q_OBJECT

            CSVRender::Cell *mParent;

        public:

            SignalHandler (CSVRender::Cell *parent);

            void connectToCommand(const ModifyPathgridCommand *command);

        public slots:

            void rebuildPathgrid();

        signals:

            void flagAsModified();
    };
}
#endif // CSM_WOLRD_PATHGRIDCOMMANDS_H
