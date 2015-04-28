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
#include "nestedtablewrapper.hpp"

class QModelIndex;
class QAbstractItemModel;

namespace CSMWorld
{
    class IdTable;
    class IdTree;
    struct RecordBase;
    struct NestedTableWrapperBase;

    class ModifyCommand : public QUndoCommand
    {
            QAbstractItemModel *mModel;
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
            std::map<int, QVariant> mValues;

        protected:

            IdTable& mModel;
            std::string mId;
            UniversalId::Type mType;

        protected:

            /// Apply modifications set via addValue.
            void applyModifications();

        public:

            CreateCommand (IdTable& model, const std::string& id, QUndoCommand *parent = 0);

            void setType (UniversalId::Type type);

            void addValue (int column, const QVariant& value);

            virtual void redo();

            virtual void undo();
    };

    class CloneCommand : public CreateCommand
    {
            std::string mIdOrigin;

        public:

            CloneCommand (IdTable& model, const std::string& idOrigin,
                          const std::string& IdDestination,
                          const UniversalId::Type type,
                          QUndoCommand* parent = 0);

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
            UniversalId::Type mType;

            // not implemented
            DeleteCommand (const DeleteCommand&);
            DeleteCommand& operator= (const DeleteCommand&);

        public:

            DeleteCommand (IdTable& model, const std::string& id,
                    UniversalId::Type type = UniversalId::Type_None, QUndoCommand *parent = 0);

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

    /// \brief Update cell ID according to x/y-coordinates
    ///
    /// \note The new value will be calculated in the first call to redo instead of the
    /// constructor to accommodate multiple coordinate-affecting commands being executed
    /// in a macro.
    class UpdateCellCommand : public QUndoCommand
    {
            IdTable& mModel;
            int mRow;
            QModelIndex mIndex;
            QVariant mNew; // invalid, if new cell ID has not been calculated yet
            QVariant mOld;

        public:

            UpdateCellCommand (IdTable& model, int row, QUndoCommand *parent = 0);

            virtual void redo();

            virtual void undo();
    };


    class NestedTableStoring
    {
        NestedTableWrapperBase* mOld;

    public:
        NestedTableStoring(const IdTree& model, const std::string& id, int parentColumn);

        ~NestedTableStoring();

    protected:

        const NestedTableWrapperBase& getOld() const;
    };

    class DeleteNestedCommand : public QUndoCommand, private NestedTableStoring
    {
            IdTree& mModel;

            std::string mId;

            int mParentColumn;

            int mNestedRow;

        public:

            DeleteNestedCommand (IdTree& model, const std::string& id, int nestedRow, int parentColumn, QUndoCommand* parent = 0);

            virtual void redo();

            virtual void undo();
    };

    class AddNestedCommand : public QUndoCommand, private NestedTableStoring
    {
            IdTree& mModel;

            std::string mId;

            int mNewRow;

            int mParentColumn;

        public:

            AddNestedCommand(IdTree& model, const std::string& id, int nestedRow, int parentColumn, QUndoCommand* parent = 0);

            virtual void redo();

            virtual void undo();
    };
}

#endif
