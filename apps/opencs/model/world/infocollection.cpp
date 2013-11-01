
#include "infocollection.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/loaddial.hpp>

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
    /// \todo adjust ID
    std::string id = reader.getHNOString ("NAME");

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
