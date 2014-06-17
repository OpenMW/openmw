
#include "refcollection.hpp"

#include <sstream>

#include <components/misc/stringops.hpp>

#include "ref.hpp"
#include "cell.hpp"
#include "universalid.hpp"
#include "record.hpp"

void CSMWorld::RefCollection::load (ESM::ESMReader& reader, int cellIndex, bool base,
    std::map<ESM::RefNum, std::string>& cache, CSMDoc::Stage::Messages& messages)
{
    Record<Cell> cell = mCells.getRecord (cellIndex);

    Cell& cell2 = base ? cell.mBase : cell.mModified;

    CellRef ref;

    bool deleted = false;

    while (ESM::Cell::getNextRef (reader, ref, deleted))
    {
        ref.mCell = cell2.mId;

        /// \todo handle moved references

        std::map<ESM::RefNum, std::string>::iterator iter = cache.find (ref.mRefNum);

        if (deleted)
        {
            if (iter==cache.end())
            {
                CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Cell,
                    mCells.getId (cellIndex));

                messages.push_back (std::make_pair (id,
                    "Attempt to delete a non-existing reference"));

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
