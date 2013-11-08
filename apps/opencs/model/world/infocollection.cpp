
#include "infocollection.hpp"

#include <stdexcept>

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

        insertRecord (record2, getIdMap().size());
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

int CSMWorld::InfoCollection::getAppendIndex (const std::string& id, UniversalId::Type type) const
{
    std::string::size_type separator = id.find_last_of ('#');

    if (separator==std::string::npos)
        throw std::runtime_error ("invalid info ID: " + id);

    std::pair<MapConstIterator, MapConstIterator> range = getTopicRange (id.substr (0, separator));

    if (range.first==range.second)
        return Collection<Info, IdAccessor<Info> >::getAppendIndex (id, type);

    int index = 0;

    for (; range.first!=range.second; ++range.first)
        if (range.first->second>index)
            index = range.first->second;

    return index+1;
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
        if (Misc::StringUtils::lowerCase (getRecord (begin->second).get().mTopicId)==topic2)
            break;

    // Find end
    MapConstIterator end = begin;

    for (; end!=getIdMap().end(); ++end)
        if (Misc::StringUtils::lowerCase (getRecord (end->second).get().mTopicId)!=topic2)
            break;

    return std::make_pair (begin, end);
}