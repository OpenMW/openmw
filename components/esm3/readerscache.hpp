#ifndef OPENMW_COMPONENTS_ESM3_READERSCACHE_H
#define OPENMW_COMPONENTS_ESM3_READERSCACHE_H

#include "esmreader.hpp"

#include <cstddef>
#include <list>
#include <map>
#include <optional>
#include <string>

#include <components/to_utf8/to_utf8.hpp>

namespace ESM
{
    class ReadersCache
    {
    private:
        enum class State
        {
            Busy,
            Free,
            Closed,
        };

        struct Item
        {
            State mState = State::Busy;
            ESMReader mReader;
            std::optional<std::filesystem::path> mName;

            Item() = default;
        };

    public:
        class BusyItem
        {
        public:
            explicit BusyItem(ReadersCache& owner, std::list<Item>::iterator item) noexcept;

            BusyItem(const BusyItem& other) = delete;

            ~BusyItem() noexcept;

            BusyItem& operator=(const BusyItem& other) = delete;

            ESMReader& operator*() const noexcept { return mItem->mReader; }

            ESMReader* operator->() const noexcept { return &mItem->mReader; }

        private:
            ReadersCache& mOwner;
            std::list<Item>::iterator mItem;
        };

        explicit ReadersCache(std::size_t capacity = 100);

        BusyItem get(std::size_t index);

        void setStatelessEncoder(const ToUTF8::StatelessUtf8Encoder& statelessEncoderPtr)
        {
            mStatelessEncoder.emplace(statelessEncoderPtr);
        }

        const ToUTF8::StatelessUtf8Encoder* getStatelessEncoder()
        {
            return mStatelessEncoder.has_value() ? &mStatelessEncoder.value() : nullptr;
        }

    private:
        const std::size_t mCapacity;
        std::map<std::size_t, std::list<Item>::iterator> mIndex;
        std::list<Item> mBusyItems;
        std::list<Item> mFreeItems;
        std::list<Item> mClosedItems;
        std::optional<ToUTF8::StatelessUtf8Encoder> mStatelessEncoder;
        inline void closeExtraReaders();

        inline void releaseItem(std::list<Item>::iterator it) noexcept;
    };
}

#endif
