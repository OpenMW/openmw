#include "refcollection.hpp"

#include <components/misc/stringops.hpp>
#include <components/esm/loadcell.hpp>

#include "ref.hpp"
#include "cell.hpp"
#include "universalid.hpp"
#include "record.hpp"

void CSMWorld::RefCollection::load (ESM::ESMReader& reader, int cellIndex, bool base,
    std::map<ESM::RefNum, std::string>& cache, CSMDoc::Messages& messages)
{
    Record<Cell> cell = mCells.getRecord (cellIndex);

    Cell& cell2 = base ? cell.mBase : cell.mModified;

    CellRef ref;
    ref.mNew = false;
    ESM::MovedCellRef mref;
    mref.mRefNum.mIndex = 0;
    bool isDeleted = false;

    while (ESM::Cell::getNextRef(reader, ref, isDeleted, true, &mref))
    {
        // Keep mOriginalCell empty when in modified (as an indicator that the
        // original cell will always be equal the current cell).
        ref.mOriginalCell = base ? cell2.mId : "";

        if (cell.get().isExterior())
        {
            // Autocalculate the cell index from coordinates first
            std::pair<int, int> index = ref.getCellIndex();

            ref.mCell = "#" + std::to_string(index.first) + " " + std::to_string(index.second);

            // Handle non-base moved references
            if (!base && mref.mRefNum.mIndex != 0)
            {
                // Moved references must have a link back to their original cell
                // See discussion: https://forum.openmw.org/viewtopic.php?f=6&t=577&start=30
                ref.mOriginalCell = cell2.mId;

                // Some mods may move references outside of the bounds, which often happens they are deleted.
                // This results in nonsensical autocalculated cell IDs, so we must use the record target cell.

                // Log a warning if the record target cell is different
                if (index.first != mref.mTarget[0] || index.second != mref.mTarget[1])
                {
                    std::string indexCell = ref.mCell;
                    ref.mCell = "#" + std::to_string(mref.mTarget[0]) + " " + std::to_string(mref.mTarget[1]);

                    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Cell, mCells.getId (cellIndex));
                    messages.add(id, "The position of the moved reference " + ref.mRefID + " (cell " + indexCell + ")"
                                     " does not match the target cell (" + ref.mCell + ")",
                                     std::string(), CSMDoc::Message::Severity_Warning);
                }
            }
        }
        else
            ref.mCell = cell2.mId;

        mref.mRefNum.mIndex = 0;

        // ignore content file number
        std::map<ESM::RefNum, std::string>::iterator iter = cache.begin();
        for (; iter != cache.end(); ++iter)
        {
            if (ref.mRefNum.mIndex == iter->first.mIndex)
                break;
        }

        if (isDeleted)
        {
            if (iter==cache.end())
            {
                CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Cell,
                    mCells.getId (cellIndex));

                messages.add (id, "Attempt to delete a non-existent reference");
                continue;
            }

            int index = getIndex (iter->second);

            Record<CellRef> record = getRecord (index);

            if (base)
            {
                removeRows (index, 1);
                cache.erase (iter);
            }
            else
            {
                record.mState = RecordBase::State_Deleted;
                setRecord (index, record);
            }

            continue;
        }

        if (iter==cache.end())
        {
            // new reference
            ref.mId = getNewId();

            Record<CellRef> record;
            record.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
            (base ? record.mBase : record.mModified) = ref;

            appendRecord (record);

            cache.insert (std::make_pair (ref.mRefNum, ref.mId));
        }
        else
        {
            // old reference -> merge
            ref.mId = iter->second;

            int index = getIndex (ref.mId);

            Record<CellRef> record = getRecord (index);
            record.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_Modified;
            (base ? record.mBase : record.mModified) = ref;

            setRecord (index, record);
        }
    }
}

std::string CSMWorld::RefCollection::getNewId()
{
    return "ref#" + std::to_string(mNextId++);
}
