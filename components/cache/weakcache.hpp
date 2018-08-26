#ifndef OPENMW_COMPONENTS_WEAKCACHE_HPP
#define OPENMW_COMPONENTS_WEAKCACHE_HPP

#include <memory>
#include <unordered_map>

namespace cache
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
            iterator(typename Map::iterator current, typename Map::iterator end);
            iterator& operator++();
            bool operator==(const iterator& other);
            bool operator!=(const iterator& other);
            StrongPtr operator*();
        private:
            typename Map::iterator mCurrent, mEnd;
            StrongPtr mPtr;
        };

        /// Stores a weak pointer to the item.
        void insert(Key key, StrongPtr value);

        /// Retrieves the item associated with the key.
        /// \return An item or null.
        StrongPtr get(Key key);

        iterator begin();
        iterator end();

    private:
        Map mData;
    };


    template <typename Key, typename T>
    WeakCache<Key, T>::iterator::iterator(typename Map::iterator current, typename Map::iterator end)
        : mCurrent(current)
        , mEnd(end)
    {
        // Move to 1st available valid item
        for ( ; mCurrent != mEnd; ++mCurrent)
        {
            mPtr = mCurrent->second.lock();
            if (mPtr) break;
        }
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::iterator& WeakCache<Key, T>::iterator::operator++()
    {
        auto next = mCurrent;
        ++next;
        return *this = iterator(next, mEnd);
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
    void WeakCache<Key, T>::insert(Key key, StrongPtr value)
    {
        mData[key] = WeakPtr(value);
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
        return iterator(mData.begin(), mData.end());
    }

    template <typename Key, typename T>
    typename WeakCache<Key, T>::iterator WeakCache<Key, T>::end()
    {
        return iterator(mData.end(), mData.end());
    }
}

#endif
