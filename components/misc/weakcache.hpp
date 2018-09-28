#ifndef OPENMW_COMPONENTS_WEAKCACHE_HPP
#define OPENMW_COMPONENTS_WEAKCACHE_HPP

#include <memory>
#include <unordered_map>
#include <vector>

namespace Misc
{
    /// \class WeakCache
    /// Provides a container to weakly store pointers to shared data.
    template <typename Key, typename T>
    class WeakCache
    {
    public:
        using WeakPtr = std::weak_ptr<T>;
        using StrongPtr = std::shared_ptr<T>;
        using Map = std::unordered_map<Key, WeakPtr>;

        class iterator
        {
        public:
            iterator(WeakCache* cache, typename Map::iterator current, typename Map::iterator end);
            iterator& operator++();
            bool operator==(const iterator& other);
            bool operator!=(const iterator& other);
            StrongPtr operator*();
        private:
            WeakCache* mCache;
            typename Map::iterator mCurrent, mEnd;
            StrongPtr mPtr;
        };

        /// Stores a weak pointer to the item.
        void insert(Key key, StrongPtr value, bool prune=true);

        /// Retrieves the item associated with the key.
        /// \return An item or null.
        StrongPtr get(Key key);

        iterator begin();
        iterator end();

        /// Removes known invalid entries
        void prune();

    private:
        Map mData;
        std::vector<Key> mDirty;
    };


    template <typename Key, typename T>
    WeakCache<Key, T>::iterator::iterator(WeakCache* cache, typename Map::iterator current, typename Map::iterator end)
        : mCache(cache)
        , mCurrent(current)
        , mEnd(end)
    {
        // Move to 1st available valid item
        for ( ; mCurrent != mEnd; ++mCurrent)
        {
            mPtr = mCurrent->second.lock();
            if (mPtr) break;
            else mCache->mDirty.push_back(mCurrent->first);
        }
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::iterator& WeakCache<Key, T>::iterator::operator++()
    {
        auto next = mCurrent;
        ++next;
        return *this = iterator(mCache, next, mEnd);
    }

    template <typename Key, typename T>
    bool WeakCache<Key, T>::iterator::operator==(const iterator& other)
    {
        return mCurrent == other.mCurrent;
    }

    template <typename Key, typename T>
    bool WeakCache<Key, T>::iterator::operator!=(const iterator& other)
    {
        return !(*this == other);
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::StrongPtr WeakCache<Key, T>::iterator::operator*()
    {
        return mPtr;
    }


    template <typename Key, typename T>
    void WeakCache<Key, T>::insert(Key key, StrongPtr value, bool shouldPrune)
    {
        mData[key] = WeakPtr(value);
        if (shouldPrune) prune();
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::StrongPtr WeakCache<Key, T>::get(Key key)
    {
        auto searchIt = mData.find(key);
        if (searchIt != mData.end())
            return searchIt->second.lock();
        else
            return StrongPtr();
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::iterator WeakCache<Key, T>::begin()
    {
        return iterator(this, mData.begin(), mData.end());
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::iterator WeakCache<Key, T>::end()
    {
        return iterator(this, mData.end(), mData.end());
    }

    template <typename Key, typename T>
    void WeakCache<Key, T>::prune()
    {
        // Remove empty entries
        for (auto& key : mDirty)
        {
            auto it = mData.find(key);
            if (it != mData.end() && it->second.use_count() == 0)
                mData.erase(it);
        }
        mDirty.clear();
    }
}

#endif
