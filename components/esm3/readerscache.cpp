#include "readerscache.hpp"

#include <stdexcept>

namespace ESM
{
    ReadersCache::BusyItem::BusyItem(ReadersCache& owner, std::list<Item>::iterator item) noexcept
        : mOwner(owner)
        , mItem(item)
    {
    }

    ReadersCache::BusyItem::~BusyItem() noexcept
    {
        mOwner.releaseItem(mItem);
    }

    ReadersCache::ReadersCache(std::size_t capacity)
        : mCapacity(capacity)
    {
    }

    ReadersCache::BusyItem ReadersCache::get(std::size_t index)
    {
        const auto indexIt = mIndex.find(index);
        std::list<Item>::iterator it;
        if (indexIt == mIndex.end())
        {
            closeExtraReaders();
            it = mBusyItems.emplace(mBusyItems.end());
            mIndex.emplace(index, it);
        }
        else
        {
            switch (indexIt->second->mState)
            {
                case State::Busy:
                    throw std::logic_error("ESMReader at index " + std::to_string(index) + " is busy");
                case State::Free:
                    it = indexIt->second;
                    mBusyItems.splice(mBusyItems.end(), mFreeItems, it);
                    break;
                case State::Closed:
                    closeExtraReaders();
                    it = indexIt->second;
                    if (it->mName.has_value())
                    {
                        it->mReader.open(*it->mName);
                        it->mName.reset();
                        it->mFileSize.reset();
                    }
                    mBusyItems.splice(mBusyItems.end(), mClosedItems, it);
                    break;
            }
            it->mState = State::Busy;
        }

        return BusyItem(*this, it);
    }

    const std::filesystem::path& ReadersCache::getName(std::size_t index) const
    {
        const auto indexIt = mIndex.find(index);
        if (indexIt == mIndex.end())
            throw std::logic_error("ESMReader at index " + std::to_string(index) + " has not been created yet");
        else
        {
            switch (indexIt->second->mState)
            {
                case State::Busy:
                case State::Free:
                    return indexIt->second->mReader.getName();
                case State::Closed:
                    if (indexIt->second->mName)
                        return *indexIt->second->mName;
                    else
                        throw std::logic_error(
                            "ESMReader at index " + std::to_string(index) + " has forgotten its filename");
                default:
                    throw std::logic_error("ESMReader at index " + std::to_string(index) + " in unknown state");
            }
        }
    }

    std::size_t ReadersCache::getFileSize(std::size_t index)
    {
        const auto indexIt = mIndex.find(index);
        if (indexIt == mIndex.end())
            return 0;
        else
        {
            switch (indexIt->second->mState)
            {
                case State::Busy:
                case State::Free:
                    if (!indexIt->second->mReader.getName().empty())
                        return indexIt->second->mReader.getFileSize();
                    else
                        throw std::logic_error(
                            "ESMReader at index " + std::to_string(index) + " has not been opened yet");
                case State::Closed:
                    if (indexIt->second->mFileSize)
                        return *indexIt->second->mFileSize;
                    else
                        throw std::logic_error(
                            "ESMReader at index " + std::to_string(index) + " has forgotten its file size");
                default:
                    throw std::logic_error("ESMReader at index " + std::to_string(index) + " in unknown state");
            }
        }
    }

    void ReadersCache::closeExtraReaders()
    {
        while (!mFreeItems.empty() && mBusyItems.size() + mFreeItems.size() + 1 > mCapacity)
        {
            const auto it = mFreeItems.begin();
            if (it->mReader.isOpen())
            {
                it->mName = it->mReader.getName();
                it->mFileSize = it->mReader.getFileSize();
                it->mReader.close();
            }
            mClosedItems.splice(mClosedItems.end(), mFreeItems, it);
            it->mState = State::Closed;
        }
    }

    void ReadersCache::releaseItem(std::list<Item>::iterator it) noexcept
    {
        assert(it->mState == State::Busy);
        if (it->mReader.isOpen())
        {
            mFreeItems.splice(mFreeItems.end(), mBusyItems, it);
            it->mState = State::Free;
        }
        else
        {
            mClosedItems.splice(mClosedItems.end(), mBusyItems, it);
            it->mState = State::Closed;
        }
    }

    void ReadersCache::clear()
    {
        mIndex.clear();
        mBusyItems.clear();
        mFreeItems.clear();
        mClosedItems.clear();
    }
}
