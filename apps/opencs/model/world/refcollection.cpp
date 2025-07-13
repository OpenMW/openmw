#include "refcollection.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <apps/opencs/model/world/collection.hpp>

#include <components/esm3/cellref.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/strings/conversion.hpp>

#include "cell.hpp"
#include "record.hpp"
#include "ref.hpp"
#include "universalid.hpp"

#include "../doc/messages.hpp"

namespace CSMWorld
{
    template <>
    void Collection<CellRef>::removeRows(int index, int count)
    {
        mRecords.erase(mRecords.begin() + index, mRecords.begin() + index + count);

        // index map is updated in RefCollection::removeRows()
    }

    template <>
    void Collection<CellRef>::insertRecord(std::unique_ptr<RecordBase> record, int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CellRef>> record2(static_cast<Record<CellRef>*>(record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin() + index, std::move(record2));

        // index map is updated in RefCollection::insertRecord()
    }
}

void CSMWorld::RefCollection::load(ESM::ESMReader& reader, int cellIndex, bool base,
    std::map<ESM::RefNum, unsigned int>& cache, CSMDoc::Messages& messages)
{
    Record<Cell> cell = mCells.getRecord(cellIndex);

    Cell& cell2 = base ? cell.mBase : cell.mModified;

    ESM::MovedCellRef mref;
    bool isDeleted = false;
    bool isMoved = false;

    while (true)
    {
        CellRef ref;
        ref.mNew = false;

        if (!ESM::Cell::getNextRef(reader, ref, isDeleted, mref, isMoved))
            break;
        if (!base && reader.getIndex() == ref.mRefNum.mContentFile)
            ref.mRefNum.mContentFile = -1;
        // Keep mOriginalCell empty when in modified (as an indicator that the
        // original cell will always be equal the current cell).
        ref.mOriginalCell = base ? cell2.mId : ESM::RefId();

        if (cell.get().isExterior())
        {
            // Autocalculate the cell index from coordinates first
            std::pair<int, int> index = ref.getCellIndex();

            ref.mCell = ESM::RefId::stringRefId(ESM::RefId::esm3ExteriorCell(index.first, index.second).toString());

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
                    ESM::RefId indexCell = ref.mCell;
                    ref.mCell = ESM::RefId::stringRefId(
                        ESM::RefId::esm3ExteriorCell(mref.mTarget[0], mref.mTarget[1]).toString());

                    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Cell, mCells.getId(cellIndex));
                    messages.add(id, "The position of the moved reference " + ref.mRefID.toDebugString() + " (cell " + indexCell.toDebugString() + ")"
                                     " does not match the target cell (" + ref.mCell.toDebugString() + ")",
                                     std::string(), CSMDoc::Message::Severity_Warning);
                }
            }
        }
        else
            ref.mCell = cell2.mId;

        auto iter = cache.find(ref.mRefNum);

        if (isMoved)
        {
            if (iter == cache.end())
            {
                CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Cell, mCells.getId(cellIndex));

                messages.add(id,
                    "Attempt to move a non-existent reference - RefNum index " + std::to_string(ref.mRefNum.mIndex)
                        + ", refID " + ref.mRefID.toDebugString() + ", content file index "
                        + std::to_string(ref.mRefNum.mContentFile),
                    /*hint*/ "", CSMDoc::Message::Severity_Warning);
                continue;
            }

            int index = getIntIndex(iter->second);

            // ensure we have the same record id for setRecord()
            ref.mId = getRecord(index).get().mId;
            ref.mIdNum = extractIdNum(ref.mId.getRefIdString());

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
            if (iter == cache.end())
            {
                CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Cell, mCells.getId(cellIndex));

                messages.add(id,
                    "Attempt to delete a non-existent reference - RefNum index " + std::to_string(ref.mRefNum.mIndex)
                        + ", refID " + ref.mRefID.getRefIdString() + ", content file index "
                        + std::to_string(ref.mRefNum.mContentFile),
                    /*hint*/ "", CSMDoc::Message::Severity_Warning);
                continue;
            }

            int index = getIntIndex(iter->second);

            if (base)
            {
                removeRows(index, 1);
                cache.erase(iter);
            }
            else
            {
                auto record = std::make_unique<Record<CellRef>>(getRecord(index));
                record->mState = RecordBase::State_Deleted;
                setRecord(index, std::move(record));
            }

            continue;
        }

        if (iter == cache.end())
        {
            // new reference
            ref.mIdNum = mNextId; // FIXME: fragile
            ref.mId = ESM::RefId::stringRefId(getNewId());

            if (!base && ref.mRefNum.mIndex >= mHighestUsedRefNum)
                mHighestUsedRefNum = ref.mRefNum.mIndex;

            cache.emplace(ref.mRefNum, ref.mIdNum);

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
            ref.mIdNum = extractIdNum(ref.mId.getRefIdString());

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

uint32_t CSMWorld::RefCollection::getNextRefNum()
{
    return ++mHighestUsedRefNum;
}

unsigned int CSMWorld::RefCollection::extractIdNum(std::string_view id) const
{
    std::string::size_type separator = id.find_last_of('#');

    if (separator == std::string::npos)
        throw std::runtime_error("invalid ref ID: " + std::string(id));

    return Misc::StringUtils::toNumeric<unsigned int>(id.substr(separator + 1), 0);
}

int CSMWorld::RefCollection::getIntIndex(unsigned int id) const
{
    int index = searchId(id);

    if (index == -1)
        throw std::runtime_error("invalid RefNum: " + std::to_string(id));

    return index;
}

int CSMWorld::RefCollection::searchId(unsigned int id) const
{
    std::map<unsigned int, int>::const_iterator iter = mRefIndex.find(id);

    if (iter == mRefIndex.end())
        return -1;

    return iter->second;
}

void CSMWorld::RefCollection::removeRows(int index, int count)
{
    Collection<CellRef>::removeRows(index, count); // erase records only

    std::map<unsigned int, int>::iterator iter = mRefIndex.begin();
    while (iter != mRefIndex.end())
    {
        if (iter->second >= index)
        {
            if (iter->second >= index + count)
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

void CSMWorld::RefCollection::appendBlankRecord(const ESM::RefId& id, UniversalId::Type type)
{
    auto record = std::make_unique<Record<CellRef>>();

    record->mState = Record<CellRef>::State_ModifiedOnly;
    record->mModified.blank();

    record->get().mId = id;
    record->get().mIdNum = extractIdNum(id.getRefIdString());
    record->get().mRefNum.mIndex = getNextRefNum();

    Collection<CellRef>::appendRecord(std::move(record));
}

void CSMWorld::RefCollection::cloneRecord(
    const ESM::RefId& origin, const ESM::RefId& destination, const UniversalId::Type type)
{
    auto copy = std::make_unique<Record<CellRef>>();
    int index = getAppendIndex(ESM::RefId(), type);

    copy->mModified = getRecord(origin).get();
    copy->mState = RecordBase::State_ModifiedOnly;

    copy->get().mId = destination;
    copy->get().mIdNum = extractIdNum(destination.getRefIdString());
    copy->get().mRefNum.mIndex = getNextRefNum();

    if (copy->get().mRefNum.hasContentFile())
    {
        mRefIndex.insert(std::make_pair(static_cast<Record<CellRef>*>(copy.get())->get().mIdNum, index));
        copy->get().mRefNum.mContentFile = -1;
    }

    insertRecord(std::move(copy), getAppendIndex(destination, type)); // call RefCollection::insertRecord()
}

int CSMWorld::RefCollection::searchId(const ESM::RefId& id) const
{
    return searchId(extractIdNum(id.getRefIdString()));
}

void CSMWorld::RefCollection::appendRecord(std::unique_ptr<RecordBase> record, UniversalId::Type type)
{
    int index = getAppendIndex(/*id*/ ESM::RefId(), type); // for CellRef records id is ignored

    mRefIndex.insert(std::make_pair(static_cast<Record<CellRef>*>(record.get())->get().mIdNum, index));

    Collection<CellRef>::insertRecord(std::move(record), index, type); // add records only
}

void CSMWorld::RefCollection::insertRecord(std::unique_ptr<RecordBase> record, int index, UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/ ESM::RefId(), type); // for CellRef records id is ignored
    unsigned int idNum = static_cast<Record<CellRef>*>(record.get())->get().mIdNum;

    Collection<CellRef>::insertRecord(std::move(record), index, type); // add records only

    if (index < size - 1)
    {
        for (std::map<unsigned int, int>::iterator iter(mRefIndex.begin()); iter != mRefIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mRefIndex.insert(std::make_pair(idNum, index));
}
