#include "refcollection.hpp"

#include <components/esm3/loadcell.hpp>

#include "ref.hpp"
#include "cell.hpp"
#include "universalid.hpp"
#include "record.hpp"

#include <string_view>

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
    ref.mNew = false;
    ESM::MovedCellRef mref;
    mref.mRefNum.mIndex = 0;
    bool isDeleted = false;
    bool isMoved = false;

    while (ESM::Cell::getNextRef(reader, ref, isDeleted, mref, isMoved))
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
            if (!base && isMoved)
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

        if (ref.mRefNum.mContentFile != -1 && !base)
        {
            ref.mRefNum.mContentFile = ref.mRefNum.mIndex >> 24;
            ref.mRefNum.mIndex &= 0x00ffffff;
        }

        unsigned int  refNum = (ref.mRefNum.mIndex & 0x00ffffff) |
            (ref.mRefNum.hasContentFile() ? ref.mRefNum.mContentFile : 0xff) << 24;

        std::map<unsigned int, unsigned int>::iterator iter = cache.find(refNum);

        if (isMoved)
        {
            if (iter == cache.end())
            {
                CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Cell,
                    mCells.getId(cellIndex));

                messages.add(id, "Attempt to move a non-existent reference - RefNum index "
                    + std::to_string(ref.mRefNum.mIndex) + ", refID " + ref.mRefID + ", content file index "
                    + std::to_string(ref.mRefNum.mContentFile),
                    /*hint*/"",
                    CSMDoc::Message::Severity_Warning);
                continue;
            }

            int index = getIntIndex(iter->second);

            // ensure we have the same record id for setRecord()
            ref.mId = getRecord(index).get().mId;
            ref.mIdNum = extractIdNum(ref.mId);

            auto record = std::make_unique<Record<CellRef>>();
            // TODO: check whether a base record be moved
            record->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
            (base ? record->mBase : record->mModified) = std::move(ref);

            // overwrite original record
            setRecord(index, std::move(record));

            continue; // NOTE: assumed moved references are not deleted at the same time
        }

        if (isDeleted)
        {
            if (iter==cache.end())
            {
                CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Cell,
                    mCells.getId (cellIndex));

                messages.add (id, "Attempt to delete a non-existent reference - RefNum index "
                        + std::to_string(ref.mRefNum.mIndex) + ", refID " + ref.mRefID + ", content file index "
                        + std::to_string(ref.mRefNum.mContentFile),
                        /*hint*/"",
                        CSMDoc::Message::Severity_Warning);
                continue;
            }

            int index = getIntIndex (iter->second);

            if (base)
            {
                removeRows (index, 1);
                cache.erase (iter);
            }
            else
            {
                auto record = std::make_unique<Record<CellRef>>(getRecord(index));
                record->mState = RecordBase::State_Deleted;
                setRecord(index, std::move(record));
            }

            continue;
        }

        if (iter==cache.end())
        {
            // new reference
            ref.mIdNum = mNextId; // FIXME: fragile
            ref.mId = getNewId();

            cache.emplace(refNum, ref.mIdNum);

            auto record = std::make_unique<Record<CellRef>>();
            record->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
            (base ? record->mBase : record->mModified) = std::move(ref);

            appendRecord(std::move(record));
        }
        else
        {
            // old reference -> merge
            int index = getIntIndex(iter->second);
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

            auto record = std::make_unique<Record<CellRef>>(getRecord(index));
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

unsigned int CSMWorld::RefCollection::extractIdNum(std::string_view id) const
{
    std::string::size_type separator = id.find_last_of('#');

    if (separator == std::string::npos)
        throw std::runtime_error("invalid ref ID: " + std::string(id));

    return static_cast<unsigned int>(std::stoi(std::string(id.substr(separator+1))));
}

int CSMWorld::RefCollection::getIntIndex (unsigned int id) const
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
    auto record = std::make_unique<Record<CellRef>>();

    record->mState = Record<CellRef>::State_ModifiedOnly;
    record->mModified.blank();

    record->get().mId = id;
    record->get().mIdNum = extractIdNum(id);

    Collection<CellRef, IdAccessor<CellRef> >::appendRecord(std::move(record));
}

void CSMWorld::RefCollection::cloneRecord (const std::string& origin,
                                           const std::string& destination,
                                           const UniversalId::Type type)
{
    auto copy = std::make_unique<Record<CellRef>>();

    copy->mModified = getRecord(origin).get();
    copy->mState = RecordBase::State_ModifiedOnly;

    copy->get().mId = destination;
    copy->get().mIdNum = extractIdNum(destination);

    insertRecord(std::move(copy), getAppendIndex(destination, type)); // call RefCollection::insertRecord()
}

int CSMWorld::RefCollection::searchId(std::string_view id) const
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
