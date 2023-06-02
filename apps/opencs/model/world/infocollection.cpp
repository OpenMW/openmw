#include "infocollection.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "components/debug/debuglog.hpp"
#include "components/esm3/infoorder.hpp"
#include "components/esm3/loaddial.hpp"
#include "components/esm3/loadinfo.hpp"

#include "collection.hpp"
#include "info.hpp"

namespace CSMWorld
{
    namespace
    {
        std::string_view getInfoTopicId(const ESM::RefId& infoId)
        {
            return parseInfoRefId(infoId).first;
        }
    }

    ESM::RefId makeCompositeInfoRefId(const ESM::RefId& topicId, const ESM::RefId& infoId)
    {
        return ESM::RefId::stringRefId(topicId.getRefIdString() + '#' + infoId.getRefIdString());
    }
}

void CSMWorld::InfoCollection::load(const Info& value, bool base)
{
    const int index = searchId(value.mId);

    if (index == -1)
    {
        // new record
        auto record = std::make_unique<Record<Info>>();
        record->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        (base ? record->mBase : record->mModified) = value;

        insertRecord(std::move(record), getSize());
    }
    else
    {
        // old record
        auto record = std::make_unique<Record<Info>>(getRecord(index));

        if (base)
            record->mBase = value;
        else
            record->setModified(value);

        setRecord(index, std::move(record));
    }
}

void CSMWorld::InfoCollection::load(
    ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue, InfoOrderByTopic& infoOrders)
{
    Info info;
    bool isDeleted = false;

    info.load(reader, isDeleted);

    const ESM::RefId id = makeCompositeInfoRefId(dialogue.mId, info.mId);

    if (isDeleted)
    {
        const int index = searchId(id);

        if (index == -1)
        {
            Log(Debug::Warning) << "Trying to delete absent info \"" << info.mId << "\" from topic \"" << dialogue.mId
                                << "\"";
            return;
        }

        if (base)
        {
            infoOrders.at(dialogue.mId).removeInfo(info.mId);
            removeRows(index, 1);
            return;
        }

        auto record = std::make_unique<Record<Info>>(getRecord(index));
        record->mState = RecordBase::State_Deleted;
        setRecord(index, std::move(record));

        return;
    }

    info.mTopicId = dialogue.mId;
    info.mOriginalId = info.mId;
    info.mId = id;

    load(info, base);

    infoOrders[dialogue.mId].insertInfo(OrderedInfo(info), isDeleted);
}

void CSMWorld::InfoCollection::sort(const InfoOrderByTopic& infoOrders)
{
    std::vector<int> order;
    order.reserve(getSize());
    for (const auto& [topicId, infoOrder] : infoOrders)
        for (const OrderedInfo& info : infoOrder.getOrderedInfo())
            order.push_back(getIndex(makeCompositeInfoRefId(topicId, info.mId)));
    reorderRowsImp(order);
}

CSMWorld::InfosRecordPtrByTopic CSMWorld::InfoCollection::getInfosByTopic() const
{
    InfosRecordPtrByTopic result;
    for (const std::unique_ptr<Record<Info>>& record : getRecords())
        result[record->get().mTopicId].push_back(record.get());
    return result;
}

int CSMWorld::InfoCollection::getAppendIndex(const ESM::RefId& id, UniversalId::Type /*type*/) const
{
    const auto lessByTopicId
        = [](std::string_view lhs, const std::unique_ptr<Record<Info>>& rhs) { return lhs < rhs->get().mTopicId; };
    const auto it = std::upper_bound(getRecords().begin(), getRecords().end(), getInfoTopicId(id), lessByTopicId);
    return static_cast<int>(it - getRecords().begin());
}

bool CSMWorld::InfoCollection::reorderRows(int baseIndex, const std::vector<int>& newOrder)
{
    const int lastIndex = baseIndex + static_cast<int>(newOrder.size()) - 1;

    if (lastIndex >= getSize())
        return false;

    if (getRecord(baseIndex).get().mTopicId != getRecord(lastIndex).get().mTopicId)
        return false;

    return reorderRowsImp(baseIndex, newOrder);
}
