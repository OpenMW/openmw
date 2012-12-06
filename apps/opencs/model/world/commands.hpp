#ifndef CSM_WOLRD_COMMANDS_H
#define CSM_WOLRD_COMMANDS_H

#include "record.hpp"

#include <string>

#include <QVariant>
#include <QUndoCommand>
#include <QModelIndex>

class QModelIndex;
class QAbstractItemModel;

namespace CSMWorld
{
    class IdTableProxyModel;
    class IdTable;
    class RecordBase;

    class ModifyCommand : public QUndoCommand
    {
            QAbstractItemModel& mModel;
            QModelIndex mIndex;
            QVariant mNew;
            QVariant mOld;

        public:

            ModifyCommand (QAbstractItemModel& model, const QModelIndex& index, const QVariant& new_,
                QUndoCommand *parent = 0);

            virtual void redo();

            virtual void undo();
    };

    class CreateCommand : public QUndoCommand
    {
            IdTableProxyModel& mModel;
            std::string mId;

        public:

            CreateCommand (IdTableProxyModel& model, const std::string& id, QUndoCommand *parent = 0);

            virtual void redo();

            virtual void undo();
    };

    class RevertCommand : public QUndoCommand
    {
            IdTable& mModel;
            std::string mId;
            RecordBase *mOld;

            // not implemented
            RevertCommand (const RevertCommand&);
            RevertCommand& operator= (const RevertCommand&);

        public:

            RevertCommand (IdTable& model, const std::string& id, QUndoCommand *parent = 0);

            virtual ~RevertCommand();

            virtual void redo();

            virtual void undo();
    };

    class DeleteCommand : public QUndoCommand
    {
            IdTable& mModel;
            std::string mId;
            RecordBase *mOld;

            // not implemented
            DeleteCommand (const DeleteCommand&);
            DeleteCommand& operator= (const DeleteCommand&);

        public:

            DeleteCommand (IdTable& model, const std::string& id, QUndoCommand *parent = 0);

            virtual ~DeleteCommand();

            virtual void redo();

            virtual void undo();
    };
}

#endif