#ifndef CSM_WOLRD_COMMANDS_H
#define CSM_WOLRD_COMMANDS_H

#include "record.hpp"

#include <string>
#include <map>
#include <vector>

#include <QVariant>
#include <QUndoCommand>
#include <QModelIndex>

#include "universalid.hpp"

class QModelIndex;
class QAbstractItemModel;

namespace CSMWorld
{
    class IdTable;
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

    class CloneCommand : public QUndoCommand
    {
            IdTable& mModel;
            std::string mIdOrigin;
            std::string mIdDestination;
            UniversalId::Type mType;
            std::map<int, QVariant> mValues;

        public:

            CloneCommand (IdTable& model, const std::string& idOrigin,
                          const std::string& IdDestination,
                          const UniversalId::Type type,
                          QUndoCommand* parent = 0);

            virtual void redo();

            virtual void undo();
    };

    class CreateCommand : public QUndoCommand
    {
            IdTable& mModel;
            std::string mId;
            UniversalId::Type mType;
            std::map<int, QVariant> mValues;

        public:

            CreateCommand (IdTable& model, const std::string& id, QUndoCommand *parent = 0);

            void setType (UniversalId::Type type);

            void addValue (int column, const QVariant& value);

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

    class ReorderRowsCommand : public QUndoCommand
    {
            IdTable& mModel;
            int mBaseIndex;
            std::vector<int> mNewOrder;

        public:

            ReorderRowsCommand (IdTable& model, int baseIndex, const std::vector<int>& newOrder);

            virtual void redo();

            virtual void undo();
    };
}

#endif