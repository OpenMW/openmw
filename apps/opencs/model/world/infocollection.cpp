#include "infocollection.hpp"

#include <stdexcept>
#include <iterator>
#include <cassert>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/loaddial.hpp>

#include <components/misc/stringops.hpp>

namespace CSMWorld
{
    template<>
    void Collection<Info, IdAccessor<Info> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in InfoCollection::removeRows()
    }

    template<>
    void Collection<Info, IdAccessor<Info> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<Info> > record2(static_cast<Record<Info>*>(record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in InfoCollection::insertRecord()
    }

    template<>
    bool Collection<Info, IdAccessor<Info> >::reorderRowsImp (int baseIndex,
        const std::vector<int>& newOrder)
    {
        if (!newOrder.empty())
        {
            int size = static_cast<int>(newOrder.size());

            // check that all indices are present
            std::vector<int> test(newOrder);
            std::sort(test.begin(), test.end());
            if (*test.begin() != 0 || *--test.end() != size-1)
                return false;

            // reorder records
            std::vector<std::unique_ptr<Record<Info> > > buffer(size);

            // FIXME: BUG: undo does not remove modified flag
            for (int i = 0; i < size; ++i)
            {
                buffer[newOrder[i]] = std::move(mRecords[baseIndex+i]);
                buffer[newOrder[i]]->setModified(buffer[newOrder[i]]->get());
            }

            std::move(buffer.begin(), buffer.end(), mRecords.begin()+baseIndex);

            // index map is updated in InfoCollection::reorderRows()
        }

        return true;
    }
}

void CSMWorld::InfoCollection::load (const Info& record, bool base)
{
    int index = searchId (record.mId);

    if (index==-1)
    {
        // new record
        std::unique_ptr<Record<Info> > record2(new Record<Info>);
        record2->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<Record<Info> > record2(new Record<Info>(getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified (record);

        setRecord (index, std::move(record2));
    }
}

int CSMWorld::InfoCollection::getInfoIndex(std::string_view id, std::string_view topic) const
{
    // find the topic first
    std::unordered_map<std::string, std::vector<std::pair<std::string, int> > >::const_iterator iter
        = mInfoIndex.find(Misc::StringUtils::lowerCase(topic));

    if (iter == mInfoIndex.end())
        return -1;

    // brute force loop
    for (std::vector<std::pair<std::string, int> >::const_iterator it = iter->second.begin();
            it != iter->second.end(); ++it)
    {
        if (Misc::StringUtils::ciEqual(it->first, id))
            return it->second;
    }

    return -1;
}

// Calling insertRecord() using index from getInsertIndex() needs to take into account of
// prev/next records; an example is deleting a record then undo
int CSMWorld::InfoCollection::getInsertIndex (const std::string& id,
        UniversalId::Type type, RecordBase *record) const
{
    if (record == nullptr)
    {
        std::string::size_type separator = id.find_last_of('#');

        if (separator == std::string::npos)
            throw std::runtime_error("invalid info ID: " + id);

        std::pair<RecordConstIterator, RecordConstIterator> range = getTopicRange(id.substr(0, separator));

        if (range.first == range.second)
            return Collection<Info, IdAccessor<Info> >::getAppendIndex(id, type);

        return std::distance(getRecords().begin(), range.second);
    }

    int index = -1;

    const Info& info = static_cast<Record<Info>*>(record)->get();
    std::string topic = info.mTopicId;

    // if the record has a prev, find its index value
    if (!info.mPrev.empty())
    {
        index = getInfoIndex(info.mPrev, topic);

        if (index != -1)
            ++index; // if prev exists, set current index to one above prev
    }

    // if prev doesn't exist or not found and the record has a next, find its index value
    if (index == -1 && !info.mNext.empty())
    {
        // if next exists, use its index as the current index
        index = getInfoIndex(info.mNext, topic);
    }

    // if next doesn't exist or not found (i.e. neither exist yet) then start a new one
    if (index == -1)
    {
        Range range = getTopicRange(topic); // getTopicRange converts topic to lower case first

        index = std::distance(getRecords().begin(), range.second);
    }

    return index;
}

bool CSMWorld::InfoCollection::reorderRows (int baseIndex, const std::vector<int>& newOrder)
{
    // check if the range is valid
    int lastIndex = baseIndex + newOrder.size() -1;

    if (lastIndex>=getSize())
        return false;

    // Check that topics match
    if (!Misc::StringUtils::ciEqual(getRecord(baseIndex).get().mTopicId,
                                    getRecord(lastIndex).get().mTopicId))
        return false;

    // reorder
    if (!Collection<Info, IdAccessor<Info> >::reorderRowsImp(baseIndex, newOrder))
        return false;

    // adjust index
    int size = static_cast<int>(newOrder.size());
    for (auto& [hash, infos] : mInfoIndex)
        for (auto& [a, b] : infos)
            if (b >= baseIndex && b < baseIndex + size)
                b = newOrder.at(b - baseIndex) + baseIndex;

    return true;
}

void CSMWorld::InfoCollection::load (ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue)
{
    Info info;
    bool isDeleted = false;

    info.load (reader, isDeleted);
    std::string id = Misc::StringUtils::lowerCase (dialogue.mId) + "#" + info.mId;

    if (isDeleted)
    {
        int index = searchId (id);

        if (index==-1)
        {
            // deleting a record that does not exist
            // ignore it for now
            /// \todo report the problem to the user
        }
        else if (base)
        {
            removeRows (index, 1);
        }
        else
        {
            std::unique_ptr<Record<Info> > record(new Record<Info>(getRecord(index)));
            record->mState = RecordBase::State_Deleted;
            setRecord (index, std::move(record));
        }
    }
    else
    {
        info.mTopicId = dialogue.mId;
        info.mId = id;
        load (info, base);
    }
}

CSMWorld::InfoCollection::Range CSMWorld::InfoCollection::getTopicRange (const std::string& topic)
    const
{
    std::string lowerTopic = Misc::StringUtils::lowerCase (topic);

    // find the topic
    std::unordered_map<std::string, std::vector<std::pair<std::string, int> > >::const_iterator iter
        = mInfoIndex.find(lowerTopic);

    if (iter == mInfoIndex.end())
        return Range (getRecords().end(), getRecords().end());

    // topic found, find the starting index
    int low = INT_MAX;
    for (std::vector<std::pair<std::string, int> >::const_iterator it = iter->second.begin();
            it != iter->second.end(); ++it)
    {
        low = std::min(low, it->second);
    }

    RecordConstIterator begin = getRecords().begin() + low;

    // Find end (one past the range)
    RecordConstIterator end = begin + iter->second.size();

    assert(static_cast<size_t>(std::distance(begin, end)) == iter->second.size());

    return Range (begin, end);
}

void CSMWorld::InfoCollection::removeDialogueInfos(const std::string& dialogueId)
{
    std::vector<int> erasedRecords;

    Range range = getTopicRange(dialogueId); // getTopicRange converts dialogueId to lower case first

    for (; range.first != range.second; ++range.first)
    {
        const Record<Info>& record = **range.first;

        if (Misc::StringUtils::ciEqual(dialogueId, record.get().mTopicId))
        {
            if (record.mState == RecordBase::State_ModifiedOnly)
            {
                erasedRecords.push_back(range.first - getRecords().begin());
            }
            else
            {
                std::unique_ptr<Record<Info> > record2(new Record<Info>(record));
                record2->mState = RecordBase::State_Deleted;
                setRecord(range.first - getRecords().begin(), std::move(record2));
            }
        }
        else
        {
            break;
        }
    }

    while (!erasedRecords.empty())
    {
        removeRows(erasedRecords.back(), 1);
        erasedRecords.pop_back();
    }
}

// FIXME: removing a record should adjust prev/next and mark those records as modified
// accordingly (also consider undo)
void CSMWorld::InfoCollection::removeRows (int index, int count)
{
    Collection<Info, IdAccessor<Info> >::removeRows(index, count); // erase records only

    for (std::unordered_map<std::string, std::vector<std::pair<std::string, int> > >::iterator iter
        = mInfoIndex.begin(); iter != mInfoIndex.end();)
    {
        for (std::vector<std::pair<std::string, int> >::iterator it = iter->second.begin();
                it != iter->second.end();)
        {
            if (it->second >= index)
            {
                if (it->second >= index+count)
                {
                    it->second -= count;
                    ++it;
                }
                else
                    iter->second.erase(it);
            }
            else
                ++it;
        }

        // check for an empty vector
        if (iter->second.empty())
            mInfoIndex.erase(iter++);
        else
            ++iter;
    }
}

void  CSMWorld::InfoCollection::appendBlankRecord (const std::string& id, UniversalId::Type type)
{
    std::unique_ptr<Record<Info> > record2(new Record<Info>);

    record2->mState = Record<Info>::State_ModifiedOnly;
    record2->mModified.blank();

    record2->get().mId = id;

    insertRecord(std::move(record2), getInsertIndex(id, type, nullptr), type); // call InfoCollection::insertRecord()
}

int CSMWorld::InfoCollection::searchId(std::string_view id) const
{
    std::string::size_type separator = id.find_last_of('#');

    if (separator == std::string::npos)
        throw std::runtime_error("invalid info ID: " + std::string(id));

    return getInfoIndex(id.substr(separator+1), id.substr(0, separator));
}

void CSMWorld::InfoCollection::appendRecord (std::unique_ptr<RecordBase> record, UniversalId::Type type)
{
    int index = getInsertIndex(static_cast<Record<Info>*>(record.get())->get().mId, type, record.get());

    insertRecord(std::move(record), index, type);
}

void CSMWorld::InfoCollection::insertRecord (std::unique_ptr<RecordBase> record, int index,
    UniversalId::Type type)
{
    int size = static_cast<int>(getRecords().size());

    std::string id = static_cast<Record<Info>*>(record.get())->get().mId;
    std::string::size_type separator = id.find_last_of('#');

    if (separator == std::string::npos)
        throw std::runtime_error("invalid info ID: " + id);

    Collection<Info, IdAccessor<Info> >::insertRecord(std::move(record), index, type); // add records only

    // adjust index
    if (index < size-1)
    {
        for (std::unordered_map<std::string, std::vector<std::pair<std::string, int> > >::iterator iter
            = mInfoIndex.begin(); iter != mInfoIndex.end(); ++iter)
        {
            for (std::vector<std::pair<std::string, int> >::iterator it = iter->second.begin();
                    it != iter->second.end(); ++it)
            {
                if (it->second >= index)
                    ++(it->second);
            }
        }
    }

    // get iterator for existing topic or a new topic
    std::string lowerId = Misc::StringUtils::lowerCase(id);
    std::pair<std::unordered_map<std::string, std::vector<std::pair<std::string, int> > >::iterator, bool> res
        = mInfoIndex.insert(
                std::make_pair(lowerId.substr(0, separator),
                               std::vector<std::pair<std::string, int> >())); // empty vector

    // insert info and index
    res.first->second.push_back(std::make_pair(lowerId.substr(separator+1), index));
}
