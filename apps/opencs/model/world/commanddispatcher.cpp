#include "commanddispatcher.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QObject>
#include <QVariant>

#include <apps/opencs/model/world/columnbase.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/idtablebase.hpp>
#include <apps/opencs/model/world/ref.hpp>
#include <apps/opencs/model/world/refcollection.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loaddial.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/strings/lower.hpp>

#include "../doc/document.hpp"

#include "commandmacro.hpp"
#include "commands.hpp"
#include "idtable.hpp"
#include "idtableproxymodel.hpp"
#include "record.hpp"

std::vector<std::string> CSMWorld::CommandDispatcher::getDeletableRecords() const
{
    std::vector<std::string> result;

    IdTable& model = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(mId));

    int stateColumnIndex = model.findColumnIndex(Columns::ColumnId_Modification);

    for (std::vector<std::string>::const_iterator iter(mSelection.begin()); iter != mSelection.end(); ++iter)
    {
        int row = model.getModelIndex(*iter, 0).row();

        // check record state
        RecordBase::State state
            = static_cast<RecordBase::State>(model.data(model.index(row, stateColumnIndex)).toInt());

        if (state == RecordBase::State_Deleted)
            continue;

        // check other columns (only relevant for a subset of the tables)
        int dialogueTypeIndex = model.searchColumnIndex(Columns::ColumnId_DialogueType);

        if (dialogueTypeIndex != -1)
        {
            int type = model.data(model.index(row, dialogueTypeIndex)).toInt();

            if (type != ESM::Dialogue::Topic && type != ESM::Dialogue::Journal)
                continue;
        }

        result.push_back(*iter);
    }

    return result;
}

std::vector<std::string> CSMWorld::CommandDispatcher::getRevertableRecords() const
{
    std::vector<std::string> result;

    IdTable& model = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(mId));

    /// \todo Reverting temporarily disabled on tables that support reordering, because
    /// revert logic currently can not handle reordering.
    if (model.getFeatures() & IdTable::Feature_ReorderWithinTopic)
        return result;

    int stateColumnIndex = model.findColumnIndex(Columns::ColumnId_Modification);

    for (std::vector<std::string>::const_iterator iter(mSelection.begin()); iter != mSelection.end(); ++iter)
    {
        int row = model.getModelIndex(*iter, 0).row();

        // check record state
        RecordBase::State state
            = static_cast<RecordBase::State>(model.data(model.index(row, stateColumnIndex)).toInt());

        if (state == RecordBase::State_BaseOnly)
            continue;

        result.push_back(*iter);
    }

    return result;
}

CSMWorld::CommandDispatcher::CommandDispatcher(
    CSMDoc::Document& document, const CSMWorld::UniversalId& id, QObject* parent)
    : QObject(parent)
    , mLocked(false)
    , mDocument(document)
    , mId(id)
{
}

void CSMWorld::CommandDispatcher::setEditLock(bool locked)
{
    mLocked = locked;
}

void CSMWorld::CommandDispatcher::setSelection(const std::vector<std::string>& selection)
{
    mSelection = selection;
    std::sort(mSelection.begin(), mSelection.end(), Misc::StringUtils::CiComp{});
}

void CSMWorld::CommandDispatcher::setExtendedTypes(const std::vector<UniversalId>& types)
{
    mExtendedTypes = types;
}

bool CSMWorld::CommandDispatcher::canDelete() const
{
    if (mLocked)
        return false;

    return getDeletableRecords().size() != 0;
}

bool CSMWorld::CommandDispatcher::canRevert() const
{
    if (mLocked)
        return false;

    return getRevertableRecords().size() != 0;
}

std::vector<CSMWorld::UniversalId> CSMWorld::CommandDispatcher::getExtendedTypes() const
{
    std::vector<CSMWorld::UniversalId> tables;

    if (mId == UniversalId::Type_Cells)
    {
        tables.push_back(mId);
        tables.emplace_back(UniversalId::Type_References);
        /// \todo add other cell-specific types
    }

    return tables;
}

void CSMWorld::CommandDispatcher::executeModify(
    QAbstractItemModel* model, const QModelIndex& index, const QVariant& newValue)
{
    if (mLocked)
        return;

    std::unique_ptr<CSMWorld::UpdateCellCommand> modifyCell;

    int columnId = model->data(index, ColumnBase::Role_ColumnId).toInt();

    if (columnId == Columns::ColumnId_PositionXPos || columnId == Columns::ColumnId_PositionYPos)
    {
        const float oldPosition = model->data(index).toFloat();

        // Modulate by cell size, update cell id if reference has been moved to a new cell
        if (std::abs(std::fmod(oldPosition, Constants::CellSizeInUnits))
                - std::abs(std::fmod(newValue.toFloat(), Constants::CellSizeInUnits))
            >= 0.5f)
        {
            IdTableProxyModel* proxy = dynamic_cast<IdTableProxyModel*>(model);

            int row = proxy ? proxy->mapToSource(index).row() : index.row();

            // This is not guaranteed to be the same as \a model, since a proxy could be used.
            IdTable& model2 = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(mId));

            int cellColumn = model2.searchColumnIndex(Columns::ColumnId_Cell);

            if (cellColumn != -1)
            {
                QModelIndex cellIndex = model2.index(row, cellColumn);

                std::string cellId = model2.data(cellIndex).toString().toUtf8().data();

                if (cellId.find('#') != std::string::npos)
                {
                    // Need to recalculate the cell
                    modifyCell = std::make_unique<UpdateCellCommand>(model2, row);
                }
            }
        }
    }

    auto modifyData = std::make_unique<CSMWorld::ModifyCommand>(*model, index, newValue);

    if (modifyCell.get())
    {
        CommandMacro macro(mDocument.getUndoStack());
        macro.push(modifyData.release());
        macro.push(modifyCell.release());
    }
    else
        mDocument.getUndoStack().push(modifyData.release());
}

void CSMWorld::CommandDispatcher::executeDelete()
{
    if (mLocked)
        return;

    std::vector<std::string> rows = getDeletableRecords();

    if (rows.empty())
        return;

    IdTable& model = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(mId));

    int columnIndex = model.findColumnIndex(Columns::ColumnId_Id);

    CommandMacro macro(mDocument.getUndoStack(), rows.size() > 1 ? "Delete multiple records" : "");
    for (std::vector<std::string>::const_iterator iter(rows.begin()); iter != rows.end(); ++iter)
    {
        std::string id = model.data(model.getModelIndex(*iter, columnIndex)).toString().toUtf8().constData();

        if (mId.getType() == UniversalId::Type_Referenceables)
        {
            macro.push(new CSMWorld::DeleteCommand(model, id,
                static_cast<CSMWorld::UniversalId::Type>(
                    model
                        .data(model.index(model.getModelIndex(id, columnIndex).row(),
                            model.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType)))
                        .toInt())));
        }
        else
            mDocument.getUndoStack().push(new CSMWorld::DeleteCommand(model, id));
    }
}

void CSMWorld::CommandDispatcher::executeRevert()
{
    if (mLocked)
        return;

    std::vector<std::string> rows = getRevertableRecords();

    if (rows.empty())
        return;

    IdTable& model = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(mId));

    int columnIndex = model.findColumnIndex(Columns::ColumnId_Id);

    CommandMacro macro(mDocument.getUndoStack(), rows.size() > 1 ? "Revert multiple records" : "");
    for (std::vector<std::string>::const_iterator iter(rows.begin()); iter != rows.end(); ++iter)
    {
        std::string id = model.data(model.getModelIndex(*iter, columnIndex)).toString().toUtf8().constData();

        macro.push(new CSMWorld::RevertCommand(model, id));
    }
}

void CSMWorld::CommandDispatcher::executeExtendedDelete()
{
    CommandMacro macro(
        mDocument.getUndoStack(), mExtendedTypes.size() > 1 ? tr("Extended delete of multiple records") : "");

    for (std::vector<UniversalId>::const_iterator iter(mExtendedTypes.begin()); iter != mExtendedTypes.end(); ++iter)
    {
        if (*iter == mId)
            executeDelete();
        else if (*iter == UniversalId::Type_References)
        {
            IdTable& model = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(*iter));

            const RefCollection& collection = mDocument.getData().getReferences();

            int size = collection.getSize();

            for (int i = size - 1; i >= 0; --i)
            {
                const Record<CellRef>& record = collection.getRecord(i);

                if (record.mState == RecordBase::State_Deleted)
                    continue;

                if (!std::binary_search(mSelection.begin(), mSelection.end(), record.get().mCell))
                    continue;

                macro.push(new CSMWorld::DeleteCommand(model, record.get().mId.getRefIdString()));
            }
        }
    }
}

void CSMWorld::CommandDispatcher::executeExtendedRevert()
{
    CommandMacro macro(
        mDocument.getUndoStack(), mExtendedTypes.size() > 1 ? tr("Extended revert of multiple records") : "");

    for (std::vector<UniversalId>::const_iterator iter(mExtendedTypes.begin()); iter != mExtendedTypes.end(); ++iter)
    {
        if (*iter == mId)
            executeRevert();
        else if (*iter == UniversalId::Type_References)
        {
            IdTable& model = dynamic_cast<IdTable&>(*mDocument.getData().getTableModel(*iter));

            const RefCollection& collection = mDocument.getData().getReferences();

            int size = collection.getSize();

            for (int i = size - 1; i >= 0; --i)
            {
                const Record<CellRef>& record = collection.getRecord(i);

                if (!std::binary_search(mSelection.begin(), mSelection.end(), record.get().mCell))
                    continue;

                macro.push(new CSMWorld::RevertCommand(model, record.get().mId.getRefIdString()));
            }
        }
    }
}
