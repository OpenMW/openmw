#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "collection.hpp"
#include "info.hpp"

namespace ESM
{
    struct Dialogue;
    class ESMReader;

    template <class T>
    class InfoOrder;
}

namespace CSMWorld
{
    using InfosRecordPtrByTopic = std::unordered_map<ESM::RefId, std::vector<const Record<Info>*>>;

    struct OrderedInfo
    {
        ESM::RefId mId;
        ESM::RefId mNext;
        ESM::RefId mPrev;

        explicit OrderedInfo(const Info& info)
            : mId(info.mOriginalId)
            , mNext(info.mNext)
            , mPrev(info.mPrev)
        {
        }
    };

    using InfoOrder = ESM::InfoOrder<OrderedInfo>;
    using InfoOrderByTopic = std::map<ESM::RefId, ESM::InfoOrder<OrderedInfo>>;

    class InfoCollection : public Collection<Info>
    {
    private:
        void load(const Info& value, bool base);

    public:
        void load(ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue, InfoOrderByTopic& infoOrder);

        void sort(const InfoOrderByTopic& infoOrders);

        InfosRecordPtrByTopic getInfosByTopic() const;

        int getAppendIndex(const ESM::RefId& id, UniversalId::Type type = UniversalId::Type_None) const override;

        bool reorderRows(int baseIndex, const std::vector<int>& newOrder) override;
    };

    ESM::RefId makeCompositeInfoRefId(const ESM::RefId& topicId, const ESM::RefId& infoId);
}

#endif
