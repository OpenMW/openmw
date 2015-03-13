
#include "commanddispatcher.hpp"

#include <algorithm>

#include <components/misc/stringops.hpp>

#include "../doc/document.hpp"

#include "idtable.hpp"
#include "record.hpp"
#include "commands.hpp"

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
: QObject (parent), mDocument (document), mId (id), mLocked (false)
{}

void CSMWorld::CommandDispatcher::setEditLock (bool locked)
{
    mLocked = locked;
}

void CSMWorld::CommandDispatcher::setSelection (const std::vector<std::string>& selection)
{
    mSelection = selection;
    std::for_each (mSelection.begin(), mSelection.end(), Misc::StringUtils::toLower);
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
        tables.push_back (UniversalId::Type_References);
        /// \todo add other cell-specific types
    }

    return tables;
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

    if (rows.size()>1)
        mDocument.getUndoStack().beginMacro (tr ("Delete multiple records"));

    for (std::vector<std::string>::const_iterator iter (rows.begin()); iter!=rows.end(); ++iter)
    {
        std::string id = model.data (model.getModelIndex (*iter, columnIndex)).
            toString().toUtf8().constData();

        mDocument.getUndoStack().push (new CSMWorld::DeleteCommand (model, id));
    }

    if (rows.size()>1)
        mDocument.getUndoStack().endMacro();
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

    if (rows.size()>1)
        mDocument.getUndoStack().beginMacro (tr ("Revert multiple records"));

    for (std::vector<std::string>::const_iterator iter (rows.begin()); iter!=rows.end(); ++iter)
    {
        std::string id = model.data (model.getModelIndex (*iter, columnIndex)).
            toString().toUtf8().constData();

        mDocument.getUndoStack().push (new CSMWorld::RevertCommand (model, id));
    }

    if (rows.size()>1)
        mDocument.getUndoStack().endMacro();
}

void CSMWorld::CommandDispatcher::executeExtendedDelete()
{
    if (mExtendedTypes.size()>1)
        mDocument.getUndoStack().beginMacro (tr ("Extended delete of multiple records"));

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

                mDocument.getUndoStack().push (
                    new CSMWorld::DeleteCommand (model, record.get().mId));
            }
        }
    }

    if (mExtendedTypes.size()>1)
        mDocument.getUndoStack().endMacro();
}

void CSMWorld::CommandDispatcher::executeExtendedRevert()
{
    if (mExtendedTypes.size()>1)
        mDocument.getUndoStack().beginMacro (tr ("Extended revert of multiple records"));

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

                mDocument.getUndoStack().push (
                    new CSMWorld::RevertCommand (model, record.get().mId));
            }
        }
    }

    if (mExtendedTypes.size()>1)
        mDocument.getUndoStack().endMacro();
}
