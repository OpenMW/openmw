#include "infocollection.hpp"

#include <stdexcept>
#include <iterator>

#include <components/esm/esmreader.hpp>
#include <components/esm/loaddial.hpp>

#include <components/misc/stringops.hpp>

void CSMWorld::InfoCollection::load (const Info& record, bool base)
{
    int index = searchId (record.mId);

    if (index==-1)
    {
        // new record
        Record<Info> record2;
        record2.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        (base ? record2.mBase : record2.mModified) = record;

        std::string topic = Misc::StringUtils::lowerCase (record2.get().mTopicId);

        if (!record2.get().mPrev.empty())
        {
            index = getInfoIndex (record2.get().mPrev, topic);

            if (index!=-1)
                ++index;
        }

        if (index==-1 && !record2.get().mNext.empty())
        {
            index = getInfoIndex (record2.get().mNext, topic);
        }

        if (index==-1)
        {
            Range range = getTopicRange (topic);

            index = std::distance (getRecords().begin(), range.second);
        }

        insertRecord (record2, index);
    }
    else
    {
        // old record
        Record<Info> record2 = getRecord (index);

        if (base)
            record2.mBase = record;
        else
            record2.setModified (record);

        setRecord (index, record2);
    }
}

int CSMWorld::InfoCollection::getInfoIndex (const std::string& id, const std::string& topic) const
{
    std::string fullId = Misc::StringUtils::lowerCase (topic) + "#" +  id;

    std::pair<RecordConstIterator, RecordConstIterator> range = getTopicRange (topic);

    for (; range.first!=range.second; ++range.first)
        if (Misc::StringUtils::ciEqual(range.first->get().mId, fullId))
            return std::distance (getRecords().begin(), range.first);

    return -1;
}

int CSMWorld::InfoCollection::getAppendIndex (const std::string& id, UniversalId::Type type) const
{
    std::string::size_type separator = id.find_last_of ('#');

    if (separator==std::string::npos)
        throw std::runtime_error ("invalid info ID: " + id);

    std::pair<RecordConstIterator, RecordConstIterator> range = getTopicRange (id.substr (0, separator));

    if (range.first==range.second)
        return Collection<Info, IdAccessor<Info> >::getAppendIndex (id, type);

    return std::distance (getRecords().begin(), range.second);
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
    return reorderRowsImp (baseIndex, newOrder);
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
            Record<Info> record = getRecord (index);
            record.mState = RecordBase::State_Deleted;
            setRecord (index, record);
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
    std::string topic2 = Misc::StringUtils::lowerCase (topic);

    std::map<std::string, int>::const_iterator iter = getIdMap().lower_bound (topic2);

    // Skip invalid records: The beginning of a topic string could be identical to another topic
    // string.
    for (; iter!=getIdMap().end(); ++iter)
    {
        std::string testTopicId =
            Misc::StringUtils::lowerCase (getRecord (iter->second).get().mTopicId);

        if (testTopicId==topic2)
            break;

        std::size_t size = topic2.size();

        if (testTopicId.size()<size || testTopicId.substr (0, size)!=topic2)
            return Range (getRecords().end(), getRecords().end());
    }

    if (iter==getIdMap().end())
        return Range (getRecords().end(), getRecords().end());

    RecordConstIterator begin = getRecords().begin()+iter->second;

    while (begin != getRecords().begin())
    {
        if (!Misc::StringUtils::ciEqual(begin->get().mTopicId, topic2))
        {
            // we've gone one too far, go back
            ++begin;
            break;
        }
        --begin;
    }

    // Find end
    RecordConstIterator end = begin;

    for (; end!=getRecords().end(); ++end)
        if (!Misc::StringUtils::ciEqual(end->get().mTopicId, topic2))
            break;

    return Range (begin, end);
}

void CSMWorld::InfoCollection::removeDialogueInfos(const std::string& dialogueId)
{
    std::string id = Misc::StringUtils::lowerCase(dialogueId);
    std::vector<int> erasedRecords;

    std::map<std::string, int>::const_iterator current = getIdMap().lower_bound(id);
    std::map<std::string, int>::const_iterator end = getIdMap().end();
    for (; current != end; ++current)
    {
        Record<Info> record = getRecord(current->second);

        if (Misc::StringUtils::ciEqual(dialogueId, record.get().mTopicId))
        {
            if (record.mState == RecordBase::State_ModifiedOnly)
            {
                erasedRecords.push_back(current->second);
            }
            else
            {
                record.mState = RecordBase::State_Deleted;
                setRecord(current->second, record);
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
