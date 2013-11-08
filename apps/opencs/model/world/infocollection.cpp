
#include "infocollection.hpp"

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

        appendRecord (record2);
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

void CSMWorld::InfoCollection::load (ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue)
{
    /// \todo put records into proper order
    std::string id = Misc::StringUtils::lowerCase (dialogue.mId) + "#" +
        reader.getHNOString ("INAM");

    if (reader.isNextSub ("DELE"))
    {
        int index = searchId (id);

        reader.skipRecord();

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
        Info record;
        record.mTopicId = dialogue.mId;
        record.mId = id;
        record.load (reader);

        load (record, base);
    }
}

std::pair<CSMWorld::InfoCollection::MapConstIterator, CSMWorld::InfoCollection::MapConstIterator>
    CSMWorld::InfoCollection::getTopicRange (const std::string& topic) const
{
    std::string topic2 = Misc::StringUtils::lowerCase (topic);

    MapConstIterator begin = getIdMap().lower_bound (topic2);

    // Skip invalid records: The beginning of a topic string could be identical to another topic
    // string.
    for (; begin!=getIdMap().end(); ++begin)
        if (getRecord (begin->second).get().mTopicId==topic)
            break;

    // Find end
    MapConstIterator end = begin;

    for (; end!=getIdMap().end(); ++end)
        if (getRecord (end->second).get().mTopicId!=topic)
            break;

    return std::make_pair (begin, end);
}