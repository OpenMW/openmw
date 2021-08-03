#include "commanddispatcher.hpp"

#include <algorithm>
#include <memory>

#include <components/misc/stringops.hpp>
#include <components/misc/constants.hpp>

#include "../doc/document.hpp"

#include "idtable.hpp"
#include "record.hpp"
#include "commands.hpp"
#include "idtableproxymodel.hpp"
#include "commandmacro.hpp"

std::vector<std::string> CSMWorld::CommandDispatcher::getDeletableRecords() const
{
    std::vector<std::string> result;

    IdTable& model = dynamic_cast<IdTable&> (*mDocument.getData().getTableModel (mId));

    int stateColumnIndex = model.findColumnIndex (Columns::ColumnId_Modification);

    for (std::vector<std::string>::const_iterator iter (mSelection.begin());
        iter!=mSelection.end(); ++iter)
    {
        int row = model.getModelIndex (*iter, 0).row();

        // check record state
        RecordBase::State state = static_cast<RecordBase::State> (
            model.data (model.index (row, stateColumnIndex)).toInt());

        if (state==RecordBase::State_Deleted)
            continue;

        // check other columns (only relevant for a subset of the tables)
        int dialogueTypeIndex = model.searchColumnIndex (Columns::ColumnId_DialogueType);

        if (dialogueTypeIndex!=-1)
        {
            int type = model.data (model.index (row, dialogueTypeIndex)).toInt();

            if (type!=ESM::Dialogue::Topic && type!=ESM::Dialogue::Journal)
                continue;
        }

        result.push_back (*iter);
    }

    return result;
}

std::vector<std::string> CSMWorld::CommandDispatcher::getRevertableRecords() const
{
    std::vector<std::string> result;

    IdTable& model = dynamic_cast<IdTable&> (*mDocument.getData().getTableModel (mId));

    /// \todo Reverting temporarily disabled on tables that support reordering, because
    /// revert logic currently can not handle reordering.
    if (model.getFeatures() & IdTable::Feature_ReorderWithinTopic)
        return result;

    int stateColumnIndex = model.findColumnIndex (Columns::ColumnId_Modification);

    for (std::vector<std::string>::const_iterator iter (mSelection.begin());
        iter!=mSelection.end(); ++iter)
    {
        int row = model.getModelIndex (*iter, 0).row();

        // check record state
        RecordBase::State state = static_cast<RecordBase::State> (
            model.data (model.index (row, stateColumnIndex)).toInt());

        if (state==RecordBase::State_BaseOnly)
            continue;

        result.push_back (*iter);
    }

    return result;
}

CSMWorld::CommandDispatcher::CommandDispatcher (CSMDoc::Document& document,
    const CSMWorld::UniversalId& id, QObject *parent)
: QObject (parent), mLocked (false), mDocument (document), mId (id)
{}

void CSMWorld::CommandDispatcher::setEditLock (bool locked)
{
    mLocked = locked;
}

void CSMWorld::CommandDispatcher::setSelection (const std::vector<std::string>& selection)
{
    mSelection = selection;
    std::for_each (mSelection.begin(), mSelection.end(), Misc::StringUtils::lowerCaseInPlace);
    std::sort (mSelection.begin(), mSelection.end());
}

void CSMWorld::CommandDispatcher::setExtendedTypes (const std::vector<UniversalId>& types)
{
    mExtendedTypes = types;
}

bool CSMWorld::CommandDispatcher::canDelete() const
{
    if (mLocked)
        return false;

    return getDeletableRecords().size()!=0;
}

bool CSMWorld::CommandDispatcher::canRevert() const
{
    if (mLocked)
        return false;

    return getRevertableRecords().size()!=0;
}

std::vector<CSMWorld::UniversalId> CSMWorld::CommandDispatcher::getExtendedTypes() const
{
    std::vector<CSMWorld::UniversalId> tables;

    if (mId==UniversalId::Type_Cells)
    {
        tables.push_back (mId);
        tables.emplace_back(UniversalId::Type_References);
        /// \todo add other cell-specific types
    }

    return tables;
}

void CSMWorld::CommandDispatcher::executeModify (QAbstractItemModel *model, const QModelIndex& index, const QVariant& new_)
{
    if (mLocked)
        return;

    std::unique_ptr<CSMWorld::CloneCommand> clonedData;
    std::unique_ptr<CSMWorld::DeleteCommand> deleteData;

    std::string newId;

    std::unique_ptr<CSMWorld::ModifyCommand> modifyData;
    std::unique_ptr<CSMWorld::UpdateCellCommand> modifyCell;

    int columnId = model->data (index, ColumnBase::Role_ColumnId).toInt();

    int stateColumn = dynamic_cast<CSMWorld::IdTable&>(*model).findColumnIndex(Columns::ColumnId_Modification);

    CSMWorld::IdTable& table = dynamic_cast<CSMWorld::IdTable&>(*model); // for getId()
    QModelIndex stateIndex = table.getModelIndex(table.getId(index.row()), stateColumn);
    RecordBase::State state = static_cast<RecordBase::State> (model->data(stateIndex).toInt());

    // This is not guaranteed to be the same as \a model, since a proxy could be used.
    IdTable& model2 = dynamic_cast<IdTable&> (*mDocument.getData().getTableModel(mId));

    // DeleteCommand triggers a signal to the whole row from IdTable::setData(), so ignore the second call
    if (state != RecordBase::State_Deleted &&
        (columnId==Columns::ColumnId_PositionXPos || columnId==Columns::ColumnId_PositionYPos))
    {
        const float oldPosition = model->data (index).toFloat();

        // Modulate by cell size, update cell id if reference has been moved to a new cell
        if (std::abs(std::fmod(oldPosition, Constants::CellSizeInUnits))
            - std::abs(std::fmod(new_.toFloat(), Constants::CellSizeInUnits)) >= 0.5f)
        {
            IdTableProxyModel *proxy = dynamic_cast<IdTableProxyModel *> (model);

            int row = proxy ? proxy->mapToSource (index).row() : index.row();

            int cellColumn = model2.searchColumnIndex (Columns::ColumnId_Cell);

            if (cellColumn != -1)
            {
                QModelIndex cellIndex = model2.index (row, cellColumn);

                std::string cellId = model2.data (cellIndex).toString().toUtf8().data();

                if (cellId.find ('#')!=std::string::npos)
                {
                    RefCollection& collection = mDocument.getData().getReferences();
                    newId = collection.getNewId();

                    clonedData.reset(new CloneCommand(table,
                                table.getId(row),
                                newId,
                                CSMWorld::UniversalId::Type_Reference));

                    deleteData.reset(new DeleteCommand(table,
                                table.getId(row),
                                CSMWorld::UniversalId::Type_Reference));
                }
            }
        }
    }

    if (!clonedData.get())
    {
        // DeleteCommand will trigger executeModify after setting the state to State_Deleted
        // from CommandDelegate::setModelDataImp() - ignore
        if (state != RecordBase::State_Deleted)
            modifyData.reset(new CSMWorld::ModifyCommand(*model, index, new_));
    }

    if (clonedData.get())
    {
        CommandMacro macro (mDocument.getUndoStack());
        macro.push(clonedData.release());
        macro.push(deleteData.release());

        // cannot do these earlier because newIndex is not available until CloneCommand is executed
        QModelIndex newIndex = model2.getModelIndex (newId, index.column());
        modifyData.reset (new CSMWorld::ModifyCommand (*model, newIndex, new_));
        macro.push(modifyData.release());

        // once the data is updated update the cell location
        modifyCell.reset(new UpdateCellCommand(model2, newIndex.row()));
        macro.push(modifyCell.release());
    }
    else if (!clonedData.get() && modifyData.get())
        mDocument.getUndoStack().push (modifyData.release());
}

void CSMWorld::CommandDispatcher::executeDelete()
{
    if (mLocked)
        return;

    std::vector<std::string> rows = getDeletableRecords();

    if (rows.empty())
        return;

    IdTable& model = dynamic_cast<IdTable&> (*mDocument.getData().getTableModel (mId));

    int columnIndex = model.findColumnIndex (Columns::ColumnId_Id);

    CommandMacro macro (mDocument.getUndoStack(), rows.size()>1 ? "Delete multiple records" : "");
    for (std::vector<std::string>::const_iterator iter (rows.begin()); iter!=rows.end(); ++iter)
    {
        std::string id = model.data (model.getModelIndex (*iter, columnIndex)).
            toString().toUtf8().constData();

        if (mId.getType() == UniversalId::Type_Referenceables)
        {
            macro.push (new CSMWorld::DeleteCommand (model, id,
                static_cast<CSMWorld::UniversalId::Type>(model.data (model.index (
                    model.getModelIndex (id, columnIndex).row(),
                    model.findColumnIndex (CSMWorld::Columns::ColumnId_RecordType))).toInt())));
        }
        else
            mDocument.getUndoStack().push (new CSMWorld::DeleteCommand (model, id));
    }
}

void CSMWorld::CommandDispatcher::executeRevert()
{
    if (mLocked)
        return;

    std::vector<std::string> rows = getRevertableRecords();

    if (rows.empty())
        return;

    IdTable& model = dynamic_cast<IdTable&> (*mDocument.getData().getTableModel (mId));

    int columnIndex = model.findColumnIndex (Columns::ColumnId_Id);

    CommandMacro macro (mDocument.getUndoStack(), rows.size()>1 ? "Revert multiple records" : "");
    for (std::vector<std::string>::const_iterator iter (rows.begin()); iter!=rows.end(); ++iter)
    {
        std::string id = model.data (model.getModelIndex (*iter, columnIndex)).
            toString().toUtf8().constData();

        macro.push (new CSMWorld::RevertCommand (model, id));
    }
}

void CSMWorld::CommandDispatcher::executeExtendedDelete()
{
    CommandMacro macro (mDocument.getUndoStack(), mExtendedTypes.size()>1 ? tr ("Extended delete of multiple records") : "");

    for (std::vector<UniversalId>::const_iterator iter (mExtendedTypes.begin());
        iter!=mExtendedTypes.end(); ++iter)
    {
        if (*iter==mId)
            executeDelete();
        else if (*iter==UniversalId::Type_References)
        {
            IdTable& model = dynamic_cast<IdTable&> (
                *mDocument.getData().getTableModel (*iter));

            const RefCollection& collection = mDocument.getData().getReferences();

            int size = collection.getSize();

            for (int i=size-1; i>=0; --i)
            {
                const Record<CellRef>& record = collection.getRecord (i);

                if (record.mState==RecordBase::State_Deleted)
                    continue;

                if (!std::binary_search (mSelection.begin(), mSelection.end(),
                    Misc::StringUtils::lowerCase (record.get().mCell)))
                    continue;

                macro.push (new CSMWorld::DeleteCommand (model, record.get().mId));
            }
        }
    }
}

void CSMWorld::CommandDispatcher::executeExtendedRevert()
{
    CommandMacro macro (mDocument.getUndoStack(), mExtendedTypes.size()>1 ? tr ("Extended revert of multiple records") : "");

    for (std::vector<UniversalId>::const_iterator iter (mExtendedTypes.begin());
        iter!=mExtendedTypes.end(); ++iter)
    {
        if (*iter==mId)
            executeRevert();
        else if (*iter==UniversalId::Type_References)
        {
            IdTable& model = dynamic_cast<IdTable&> (
                *mDocument.getData().getTableModel (*iter));

            const RefCollection& collection = mDocument.getData().getReferences();

            int size = collection.getSize();

            for (int i=size-1; i>=0; --i)
            {
                const Record<CellRef>& record = collection.getRecord (i);

                if (!std::binary_search (mSelection.begin(), mSelection.end(),
                    Misc::StringUtils::lowerCase (record.get().mCell)))
                    continue;

                macro.push (new CSMWorld::RevertCommand (model, record.get().mId));
            }
        }
    }
}
