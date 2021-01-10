#ifndef CSM_WOLRD_COMMANDS_H
#define CSM_WOLRD_COMMANDS_H

#include "record.hpp"

#include <string>
#include <map>
#include <memory>
#include <vector>

#include <QVariant>
#include <QUndoCommand>
#include <QModelIndex>

#include "columnimp.hpp"
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

    class TouchCommand : public QUndoCommand
    {
        public:

            TouchCommand(IdTable& model, const std::string& id, QUndoCommand* parent=nullptr);

            void redo() override;
            void undo() override;

        private:

            IdTable& mTable;
            std::string mId;
            std::unique_ptr<RecordBase> mOld;

            bool mChanged;
    };

    /// \brief Adds LandTexture records and modifies texture indices as needed.
    ///
    /// LandTexture records are different from other types of records, because
    /// they only effect the current plugin. Thus, when modifying or copying
    /// a Land record, all of the LandTexture records referenced need to be
    /// added to the current plugin. Since these newly added LandTextures could
    /// have indices that conflict with pre-existing LandTextures in the current
    /// plugin, the indices might have to be changed, both for the newly added
    /// LandRecord and within the Land record.
    class ImportLandTexturesCommand : public QUndoCommand
    {
        public:

            ImportLandTexturesCommand(IdTable& landTable, IdTable& ltexTable,
                QUndoCommand* parent);

            void redo() override;
            void undo() override;

        protected:

            using DataType = LandTexturesColumn::DataType;

            virtual const std::string& getOriginId() const = 0;
            virtual const std::string& getDestinationId() const = 0;

            virtual void onRedo() = 0;
            virtual void onUndo() = 0;

            IdTable& mLands;
            IdTable& mLtexs;
            DataType mOld;
            int mOldState;
            std::vector<std::string> mCreatedTextures;
    };

    /// \brief This command is used to fix LandTexture records and texture
    ///     indices after cloning a Land. See ImportLandTexturesCommand for
    ///     details.
    class CopyLandTexturesCommand : public ImportLandTexturesCommand
    {
        public:

            CopyLandTexturesCommand(IdTable& landTable, IdTable& ltexTable, const std::string& origin,
                const std::string& dest, QUndoCommand* parent = nullptr);

        private:

            const std::string& getOriginId() const override;
            const std::string& getDestinationId() const override;

            void onRedo() override {}
            void onUndo() override {}

            std::string mOriginId;
            std::string mDestId;
    };

    /// \brief This command brings a land record into the current plugin, adding
    ///     LandTexture records and modifying texture indices as needed.
    /// \note See ImportLandTextures for more details.
    class TouchLandCommand : public ImportLandTexturesCommand
    {
        public:

            TouchLandCommand(IdTable& landTable, IdTable& ltexTable,
                const std::string& id, QUndoCommand* parent = nullptr);

        private:

            const std::string& getOriginId() const override;
            const std::string& getDestinationId() const override;

            void onRedo() override;
            void onUndo() override;

            std::string mId;
            std::unique_ptr<RecordBase> mOld;

            bool mChanged;
    };

    class ModifyCommand : public QUndoCommand
    {
            QAbstractItemModel *mModel;
            QModelIndex mIndex;
            QVariant mNew;
            QVariant mOld;

            bool mHasRecordState;
            QModelIndex mRecordStateIndex;
            CSMWorld::RecordBase::State mOldRecordState;

        public:

            ModifyCommand (QAbstractItemModel& model, const QModelIndex& index, const QVariant& new_,
                QUndoCommand *parent = nullptr);

            void redo() override;

            void undo() override;
    };

    class CreateCommand : public QUndoCommand
    {
            std::map<int, QVariant> mValues;
            std::map<int, std::pair<int, QVariant> > mNestedValues;
            ///< Parameter order: a parent column, a nested column, a data.
            ///< A nested row has index of 0.

        protected:

            IdTable& mModel;
            std::string mId;
            UniversalId::Type mType;

        protected:

            /// Apply modifications set via addValue.
            void applyModifications();

        public:

            CreateCommand (IdTable& model, const std::string& id, QUndoCommand *parent = nullptr);

            void setType (UniversalId::Type type);

            void addValue (int column, const QVariant& value);

            void addNestedValue(int parentColumn, int nestedColumn, const QVariant &value);

            void redo() override;

            void undo() override;
    };

    class CloneCommand : public CreateCommand
    {
            std::string mIdOrigin;
            std::vector<std::pair<int, QVariant>> mOverrideValues;

        public:

            CloneCommand (IdTable& model, const std::string& idOrigin,
                          const std::string& IdDestination,
                          const UniversalId::Type type,
                          QUndoCommand* parent = nullptr);

            void redo() override;

            void undo() override;

            void setOverrideValue(int column, QVariant value);
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

            RevertCommand (IdTable& model, const std::string& id, QUndoCommand *parent = nullptr);

            virtual ~RevertCommand();

            void redo() override;

            void undo() override;
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
                    UniversalId::Type type = UniversalId::Type_None, QUndoCommand *parent = nullptr);

            virtual ~DeleteCommand();

            void redo() override;

            void undo() override;
    };

    class ReorderRowsCommand : public QUndoCommand
    {
            IdTable& mModel;
            int mBaseIndex;
            std::vector<int> mNewOrder;

        public:

            ReorderRowsCommand (IdTable& model, int baseIndex, const std::vector<int>& newOrder);

            void redo() override;

            void undo() override;
    };

    class CreatePathgridCommand : public CreateCommand
    {
        public:

            CreatePathgridCommand(IdTable& model, const std::string& id, QUndoCommand *parent = nullptr);

            void redo() override;
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

            UpdateCellCommand (IdTable& model, int row, QUndoCommand *parent = nullptr);

            void redo() override;

            void undo() override;
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

            // The command to redo/undo the Modified status of a record
            ModifyCommand *mModifyParentCommand;

        public:

            DeleteNestedCommand (IdTree& model, const std::string& id, int nestedRow, int parentColumn, QUndoCommand* parent = nullptr);

            void redo() override;

            void undo() override;
    };

    class AddNestedCommand : public QUndoCommand, private NestedTableStoring
    {
            IdTree& mModel;

            std::string mId;

            int mNewRow;

            int mParentColumn;

            // The command to redo/undo the Modified status of a record
            ModifyCommand *mModifyParentCommand;

        public:

            AddNestedCommand(IdTree& model, const std::string& id, int nestedRow, int parentColumn, QUndoCommand* parent = nullptr);

            void redo() override;

            void undo() override;
    };
}

#endif
