#ifndef OPENMW_COMPONENTS_FLATMAP_HPP
#define OPENMW_COMPONENTS_FLATMAP_HPP

#include <memory>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <utility>

namespace Misc
{
    /// \class IndexedVector
    /// Works like std::unordered_map but storage is std::vector and uses insertion order.
    template <typename Key, typename T, typename THasher = std::hash<Key>>
    class IndexedVector
    {
        using ElementType = std::pair<Key, T>;

    private:
        using Storage = std::vector<ElementType>;
        using LookupTable = std::unordered_map<Key, typename Storage::difference_type, THasher>;

    public:
        using iterator = typename Storage::iterator;
        using const_iterator = typename Storage::const_iterator;
        using value_type = typename Storage::value_type;

        void clear()
        {
            mData.clear();
            mLookup.clear();
        }

        size_t size() const noexcept
        {
            return mData.size();
        }

        /// Inserts the element at the back with move semantics.
        std::pair<iterator, bool> insert(const value_type& value)
        {
            auto it = find(value.first);
            if (it != std::end(mData))
            {
                return { it, false };
            }
            else
            {
                const auto& key = value.first;
                it = mData.emplace(std::end(mData), value);
                mLookup.emplace(key, std::distance(std::begin(mData), it));
                return { it, true };
            }
        }

        std::pair<iterator, bool> insert(value_type&& value)
        {
            auto it = find(value.first);
            if (it != std::end(mData))
            {
                return { it, false };
            }
            else
            {
                const auto& key = value.first;
                it = mData.emplace(std::end(mData), std::move(value));
                mLookup.emplace(key, std::distance(std::begin(mData), it));
                return { it, true };
            }
        }

        /// Inserts the element at the back
        template<typename ...TArgs>
        std::pair<iterator, bool> emplace(TArgs&& ...args)
        {
            value_type value{ std::forward<TArgs>(args)... };
            auto it = find(value.first);
            if (it != std::end(mData))
            {
                return { it, false };
            }
            else
            {
                const auto& key = value.first;
                it = mData.emplace(std::end(mData), std::move(value));
                mLookup.emplace(key, std::distance(std::begin(mData), it));
                return { it, true };
            }
        }

        /// Erases a single element, iterators are invalidated after this call.
        /// The returned iterator points to the value after the removed element.
        iterator erase(iterator it) noexcept
        {
            return eraseImpl(it);
        }

        iterator erase(const_iterator it) noexcept
        {
            return eraseImpl(it);
        }

        /// Erases a single element by key, iterators are invalidated after this call.
        /// The returned iterator points to the value after the removed element.
        iterator erase(const Key& key) noexcept
        {
            auto it = find(key);
            if (it == end())
                return it;
            return erase(it);
        }

        template<class K>
        iterator find(const K& key)
        {
            auto it = mLookup.find(key);
            if (it == std::end(mLookup))
                return end();
            return std::begin(mData) + it->second;
        }

        template<class K>
        const_iterator find(const K& key) const
        {
            auto it = mLookup.find(key);
            if (it == std::end(mLookup))
                return end();
            return std::begin(mData) + it->second;
        }

        iterator begin()
        {
            return mData.begin();
        }

        const_iterator begin() const
        {
            return mData.begin();
        }

        const_iterator cbegin() const
        {
            return mData.cbegin();
        }

        iterator end()
        {
            return mData.end();
        }

        const_iterator end() const
        {
            return mData.end();
        }

        const_iterator cend() const
        {
            return mData.cend();
        }

    private:
        template<typename TIterator>
        iterator eraseImpl(TIterator&& it) noexcept
        {
            const auto lookupIt = mLookup.find(it->first);
            if (lookupIt == std::end(mLookup))
                return end();
            const auto index = lookupIt->second;
            mLookup.erase(lookupIt);
            // Adjust indices by one.
            for (auto& [_, idx] : mLookup)
            {
                if (idx > index)
                    idx--;
            }
            return mData.erase(it);
        }

    private:
        Storage mData;
        LookupTable mLookup;
    };

}

#endif
