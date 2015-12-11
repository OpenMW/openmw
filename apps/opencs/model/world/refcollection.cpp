#include "refcollection.hpp"

#include <iostream>

#include <components/misc/stringops.hpp>
#include <components/esm/loadcell.hpp>

#include "ref.hpp"
#include "cell.hpp"
#include "universalid.hpp"
#include "record.hpp"

namespace CSMWorld
{
    template<>
    void Collection<CellRef, IdAccessor<CellRef> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in RefCollection::removeRows()
    }

    template<>
    void Collection<CellRef, IdAccessor<CellRef> >::insertRecord (std::unique_ptr<RecordBase> record, int index,
        UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CellRef> > record2(static_cast<Record<CellRef>*>(record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in RefCollection::insertRecord()
    }
}

void CSMWorld::RefCollection::load (ESM::ESMReader& reader, int cellIndex, bool base,
    std::map<unsigned int, unsigned int>& cache, CSMDoc::Messages& messages)
{
    Record<Cell> cell = mCells.getRecord (cellIndex);

    Cell& cell2 = base ? cell.mBase : cell.mModified;

    CellRef ref;
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

            ref.mCell = "#" + std::to_string(index.first) + " " + std::to_string(index.second);

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
                    std::cerr << "The Position of moved ref "
                        << ref.mRefID << " does not match the target cell" << std::endl;
                    std::cerr << "Position: #" << index.first << " " << index.second
                        <<", Target #"<< mref.mTarget[0] << " " << mref.mTarget[1] << std::endl;

                    // overwrite
                    ref.mCell = "#" + std::to_string(mref.mTarget[0]) + " " + std::to_string(mref.mTarget[1]);
                }
            }
        }
        else
            ref.mCell = cell2.mId;

        unsigned int  refNum = (ref.mRefNum.mIndex & 0x00ffffff) |
            (ref.mRefNum.hasContentFile() ? ref.mRefNum.mContentFile : 0xff) << 24;

        std::map<unsigned int, unsigned int>::iterator iter = cache.find(refNum);

        if (isDeleted)
        {
            if (iter==cache.end())
            {
                CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Cell,
                    mCells.getId (cellIndex));

                messages.add (id, "Attempt to delete a non-existing reference - RefNum index "
                        + std::to_string(ref.mRefNum.mIndex) + ", refID " + ref.mRefID + ", content file index "
                        + std::to_string(ref.mRefNum.mContentFile),
                        /*hint*/"",
                        CSMDoc::Message::Severity_Warning);
                continue;
            }

            int index = getIndex (iter->second);

            if (base)
            {
                removeRows (index, 1);
                cache.erase (iter);
            }
            else
            {
                std::unique_ptr<Record<CellRef> > record2(new Record<CellRef>(getRecord(index)));
                record2->mState = RecordBase::State_Deleted;
                setRecord(index, std::move(record2));
            }

            continue;
        }

        if (iter==cache.end())
        {
            // new reference
            ref.mIdNum = mNextId; // FIXME: fragile
            ref.mId = getNewId();

            cache.insert(std::make_pair(refNum, ref.mIdNum));

            std::unique_ptr<Record<CellRef> > record(new Record<CellRef>);
            record->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
            (base ? record->mBase : record->mModified) = std::move(ref);

            appendRecord(std::move(record));
        }
        else
        {
            // old reference -> merge
            int index = getIndex(iter->second);
#if 0
            // ref.mRefNum.mIndex     : the key
            // iter->second           : previously cached idNum for the key
            // index                  : position of the record for that idNum
            // getRecord(index).get() : record in the index position
            assert(iter->second != getRecord(index).get().mIdNum); // sanity check

            // check if the plugin used the same RefNum index for a different record
            if (ref.mRefID != getRecord(index).get().mRefID)
            {
                CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Cell, mCells.getId(cellIndex));
                messages.add(id,
                    "RefNum renamed from RefID \"" + getRecord(index).get().mRefID + "\" to \""
                    + ref.mRefID + "\" (RefNum index " + std::to_string(ref.mRefNum.mIndex) + ")",
                    /*hint*/"",
                    CSMDoc::Message::Severity_Info);
            }
#endif
            ref.mId = getRecord(index).get().mId;
            ref.mIdNum = extractIdNum(ref.mId);

            std::unique_ptr<Record<CellRef> > record(new Record<CellRef>(getRecord(index)));
            record->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_Modified;
            (base ? record->mBase : record->mModified) = std::move(ref);

            setRecord(index, std::move(record));
        }
    }
}

std::string CSMWorld::RefCollection::getNewId()
{
    return "ref#" + std::to_string(mNextId++);
}

unsigned int CSMWorld::RefCollection::extractIdNum (const std::string& id) const
{
    std::string::size_type separator = id.find_last_of('#');

    if (separator == std::string::npos)
        throw std::runtime_error("invalid ref ID: " + id);

    return static_cast<unsigned int>(std::stoi(id.substr(separator+1)));
}

int CSMWorld::RefCollection::getIndex (unsigned int id) const
{
    int index = searchId(id);

    if (index == -1)
        throw std::runtime_error("invalid RefNum: " + std::to_string(id));

    return index;
}

int CSMWorld::RefCollection::searchId (unsigned int id) const
{
    std::map<unsigned int, int>::const_iterator iter = mRefIndex.find(id);

    if (iter == mRefIndex.end())
        return -1;

    return iter->second;
}

void CSMWorld::RefCollection::removeRows (int index, int count)
{
    Collection<CellRef, IdAccessor<CellRef> >::removeRows(index, count); // erase records only

    std::map<unsigned int, int>::iterator iter = mRefIndex.begin();
    while (iter != mRefIndex.end())
    {
        if (iter->second>=index)
        {
            if (iter->second >= index+count)
            {
                iter->second -= count;
                ++iter;
            }
            else
                mRefIndex.erase(iter++);
        }
        else
            ++iter;
    }
}

void  CSMWorld::RefCollection::appendBlankRecord (const std::string& id, UniversalId::Type type)
{
    std::unique_ptr<Record<CellRef> > record2(new Record<CellRef>);

    record2->mState = Record<CellRef>::State_ModifiedOnly;
    record2->mModified.blank();

    record2->get().mId = id;
    record2->get().mIdNum = extractIdNum(id);

    Collection<CellRef, IdAccessor<CellRef> >::appendRecord(std::move(record2));
}

void CSMWorld::RefCollection::cloneRecord (const std::string& origin,
                                           const std::string& destination,
                                           const UniversalId::Type type)
{
   std::unique_ptr<Record<CellRef> > copy(new Record<CellRef>);

   copy->mModified = getRecord(origin).get();
   copy->mState = RecordBase::State_ModifiedOnly;

   copy->get().mId = destination;
   copy->get().mIdNum = extractIdNum(destination);

   insertRecord(std::move(copy), getAppendIndex(destination, type)); // call RefCollection::insertRecord()
}

int CSMWorld::RefCollection::searchId (const std::string& id) const
{
    return searchId(extractIdNum(id));
}

void CSMWorld::RefCollection::appendRecord (std::unique_ptr<RecordBase> record, UniversalId::Type type)
{
    int index = getAppendIndex(/*id*/"", type); // for CellRef records id is ignored

    mRefIndex.insert(std::make_pair(static_cast<Record<CellRef>*>(record.get())->get().mIdNum, index));

    Collection<CellRef, IdAccessor<CellRef> >::insertRecord(std::move(record), index, type); // add records only
}

void CSMWorld::RefCollection::insertRecord (std::unique_ptr<RecordBase> record, int index,
    UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // for CellRef records id is ignored
    unsigned int idNum = static_cast<Record<CellRef>*>(record.get())->get().mIdNum;

    Collection<CellRef, IdAccessor<CellRef> >::insertRecord(std::move(record), index, type); // add records only

    if (index < size-1)
    {
        for (std::map<unsigned int, int>::iterator iter(mRefIndex.begin()); iter != mRefIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mRefIndex.insert(std::make_pair(idNum, index));
}
