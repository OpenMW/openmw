#include "infoorder.hpp"

namespace ESM
{
    const std::list<OrderedInfo>* InfoOrder::findInfosByTopic(const ESM::RefId& refId) const
    {
        const auto it = mOrderByTopic.find(refId);
        if (it == mOrderByTopic.end())
            return nullptr;
        return &it->second;
    }

    void InfoOrder::insertInfo(const OrderedInfo& info)
    {
        auto it = mInfoPositions.find(info.mId);

        if (it != mInfoPositions.end() && it->second->mPrev == info.mPrev)
        {
            it->second->mNext = info.mNext;
            return;
        }

        auto& infos = mOrderByTopic[info.mTopicId];

        if (it == mInfoPositions.end())
            it = mInfoPositions.emplace(info.mId, infos.end()).first;

        std::list<OrderedInfo>::iterator& position = it->second;

        const auto insertOrSplice = [&](std::list<OrderedInfo>::const_iterator before) {
            if (position == infos.end())
                position = infos.insert(before, info);
            else
                infos.splice(before, infos, position);
        };

        if (info.mPrev.empty())
        {
            insertOrSplice(infos.begin());
            return;
        }

        const auto prevIt = mInfoPositions.find(info.mPrev);
        if (prevIt != mInfoPositions.end())
        {
            insertOrSplice(std::next(prevIt->second));
            return;
        }

        const auto nextIt = mInfoPositions.find(info.mNext);
        if (nextIt != mInfoPositions.end())
        {
            insertOrSplice(nextIt->second);
            return;
        }

        insertOrSplice(infos.end());
    }

    void InfoOrder::removeInfo(const ESM::RefId& infoRefId)
    {
        const auto it = mInfoPositions.find(infoRefId);

        if (it == mInfoPositions.end())
            return;

        const auto topicIt = mOrderByTopic.find(it->second->mTopicId);

        if (topicIt != mOrderByTopic.end())
            topicIt->second.erase(it->second);

        mInfoPositions.erase(it);
    }

    void InfoOrder::removeTopic(const ESM::RefId& topicRefId)
    {
        const auto it = mOrderByTopic.find(topicRefId);

        if (it == mOrderByTopic.end())
            return;

        for (const OrderedInfo& info : it->second)
            mInfoPositions.erase(info.mId);

        mOrderByTopic.erase(it);
    }
}
