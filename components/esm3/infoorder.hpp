#ifndef OPENMW_COMPONENTS_ESM3_INFOORDER_H
#define OPENMW_COMPONENTS_ESM3_INFOORDER_H

#include "components/esm/refid.hpp"

#include <iterator>
#include <list>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace ESM
{
    template <class T>
    class InfoOrder
    {
    public:
        const std::list<T>& getOrderedInfo() const { return mOrderedInfo; }

        template <class V>
        void insertInfo(V&& value, bool deleted)
        {
            static_assert(std::is_same_v<std::decay_t<V>, T>);

            auto it = mInfoPositions.find(value.mId);

            if (it != mInfoPositions.end())
            {
                bool samePrev = it->second.mPosition->mPrev == value.mPrev;
                *it->second.mPosition = std::forward<V>(value);
                it->second.mDeleted = deleted;
                if (samePrev)
                    return;
            }
            else
                it = mInfoPositions.emplace(value.mId, Item{ .mPosition = mOrderedInfo.end(), .mDeleted = deleted })
                         .first;

            Item& item = it->second;

            auto before = mOrderedInfo.begin();
            if (!value.mPrev.empty())
            {
                const auto prevIt = mInfoPositions.find(value.mPrev);
                if (prevIt != mInfoPositions.end())
                    before = std::next(prevIt->second.mPosition);
                else
                    before = mOrderedInfo.end();
            }
            if (item.mPosition == mOrderedInfo.end())
                item.mPosition = mOrderedInfo.insert(before, std::forward<V>(value));
            else
                mOrderedInfo.splice(before, mOrderedInfo, item.mPosition);
        }

        void removeInfo(const RefId& infoRefId)
        {
            const auto it = mInfoPositions.find(infoRefId);

            if (it == mInfoPositions.end())
                return;

            mOrderedInfo.erase(it->second.mPosition);
            mInfoPositions.erase(it);
        }

        void removeDeleted()
        {
            for (auto it = mInfoPositions.begin(); it != mInfoPositions.end();)
            {
                if (!it->second.mDeleted)
                {
                    ++it;
                    continue;
                }

                mOrderedInfo.erase(it->second.mPosition);
                it = mInfoPositions.erase(it);
            }
        }

        void extractOrderedInfo(std::list<T>& info)
        {
            info = mOrderedInfo;
            mInfoPositions.clear();
        }

    private:
        struct Item
        {
            typename std::list<T>::iterator mPosition;
            bool mDeleted = false;
        };

        std::list<T> mOrderedInfo;
        std::unordered_map<RefId, Item> mInfoPositions;
    };
}

#endif
