#include "refcollection.hpp"

#include <sstream>

#include <components/debug/debuglog.hpp>
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
    bool isDeleted = false;

    // hack to initialise mindex
    while (!(mref.mRefNum.mIndex = 0) && ESM::Cell::getNextRef(reader, ref, isDeleted, true, &mref))
    {
        // Keep mOriginalCell empty when in modified (as an indicator that the
        // original cell will always be equal the current cell).
        ref.mOriginalCell = base ? cell2.mId : "";

        if (cell.get().isExterior())
        {
            // ignoring moved references sub-record; instead calculate cell from coordinates
            std::pair<int, int> index = ref.getCellIndex();

            std::ostringstream stream;
            stream << "#" << index.first << " " << index.second;

            ref.mCell = stream.str();

            if (!base &&                  // don't try to update base records
                mref.mRefNum.mIndex != 0) // MVRF tag found
            {
                // there is a requirement for a placeholder where the original object was
                //
                // see the forum discussions here for more details:
                // https://forum.openmw.org/viewtopic.php?f=6&t=577&start=30
                ref.mOriginalCell = cell2.mId;

                // It is not always possibe to ignore moved references sub-record and
                // calculate from coordinates. Some mods may place the ref in positions
                // outside normal bounds, resulting in non sensical cell id's.  This often
                // happens if the moved ref was deleted.
                //
                // Use the target cell from the MVRF tag but if different output an error
                // message
                if (index.first != mref.mTarget[0] || index.second != mref.mTarget[1])
                {
                    Log(Debug::Warning) << "Warning: the Position of moved ref "
                        << ref.mRefID << " does not match the target cell";
                    Log(Debug::Warning) << "Position: #" << index.first << " " << index.second
                        <<", Target #"<< mref.mTarget[0] << " " << mref.mTarget[1];

                    stream.clear();
                    stream << "#" << mref.mTarget[0] << " " << mref.mTarget[1];
                    ref.mCell = stream.str(); // overwrite
                }
            }
        }
        else
            ref.mCell = cell2.mId;

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

                messages.add (id, "Attempt to delete a non-existing reference");
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
    std::ostringstream stream;
    stream << "ref#" << mNextId++;
    return stream.str();
}
