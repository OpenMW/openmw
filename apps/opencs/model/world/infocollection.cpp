#include "infocollection.hpp"

#include <memory>
#include <string>
#include <utility>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loaddial.hpp>

#include "collection.hpp"
#include "info.hpp"

bool CSMWorld::InfoCollection::load(const Info& record, bool base)
{
    const int index = searchId(record.mId);

    if (index == -1)
    {
        // new record
        auto record2 = std::make_unique<Record<Info>>();
        record2->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        appendRecord(std::move(record2));

        return true;
    }
    else
    {
        // old record
        auto record2 = std::make_unique<Record<Info>>(getRecord(index));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        setRecord(index, std::move(record2));

        return false;
    }
}

void CSMWorld::InfoCollection::load(
    ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue, InfosByTopic& infosByTopic)
{
    Info info;
    bool isDeleted = false;

    info.load(reader, isDeleted);
    const ESM::RefId id = ESM::RefId::stringRefId(dialogue.mId.getRefIdString() + "#" + info.mId.getRefIdString());

    if (isDeleted)
    {
        int index = searchId(id);

        if (index == -1)
        {
            // deleting a record that does not exist
            // ignore it for now
            /// \todo report the problem to the user
        }
        else if (base)
        {
            removeRows(index, 1);
        }
        else
        {
            auto record = std::make_unique<Record<Info>>(getRecord(index));
            record->mState = RecordBase::State_Deleted;
            setRecord(index, std::move(record));
        }
    }
    else
    {
        info.mTopicId = dialogue.mId;
        info.mOriginalId = info.mId;
        info.mId = id;

        if (load(info, base))
            infosByTopic[dialogue.mId].push_back(info.mId);
    }
}

std::vector<CSMWorld::Record<CSMWorld::Info>*> CSMWorld::InfoCollection::getTopicInfos(const ESM::RefId& topic) const
{
    std::vector<CSMWorld::Record<CSMWorld::Info>*> result;
    for (const std::unique_ptr<Record<Info>>& record : getRecords())
        if (record->mBase.mTopicId == topic)
            result.push_back(record.get());
    return result;
}
