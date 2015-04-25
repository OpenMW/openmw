
#include "refcollection.hpp"

#include <sstream>
#include <iostream> // FIXME: debug only

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

    bool deleted = false;
    ESM::MovedCellRef mref;

    // hack to initialise mindex
    while (!(mref.mRefNum.mIndex = 0) && ESM::Cell::getNextRef (reader, ref, deleted, true, &mref))
    {
        // Keep mOriginalCell empty when in modified (as an indicator that the
        // original cell will always be equal the current cell).
        ref.mOriginalCell = base ? cell2.mId : "";

        if (mref.mRefNum.mIndex != 0 &&
            ((int)std::floor(ref.mPos.pos[0]/8192) != mref.mTarget[0] ||
             (int)std::floor(ref.mPos.pos[1]/8192) != mref.mTarget[1]))
        {
            //std::cout <<"refcollection  #" << mref.mTarget[0] << " " << mref.mTarget[1] << std::endl;
        }

        if (cell.get().isExterior())
        {
            // ignoring moved references sub-record; instead calculate cell from coordinates
            std::pair<int, int> index = ref.getCellIndex();

            std::ostringstream stream;
            if (mref.mRefNum.mIndex)
            {
                stream << "#" << mref.mTarget[0] << " " << mref.mTarget[1];
                //std::cout <<"refcollection " + stream.str() << std::endl;
            }
            else
                stream << "#" << index.first << " " << index.second;

            ref.mCell = stream.str();
        }
        else
            ref.mCell = cell2.mId;

        std::map<ESM::RefNum, std::string>::iterator iter = cache.find (ref.mRefNum);

        if (deleted)
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

            if (record.mState==RecordBase::State_BaseOnly)
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
