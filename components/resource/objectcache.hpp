// Resource ObjectCache for OpenMW, forked from osgDB ObjectCache by Robert Osfield, see copyright notice below.
// Changes:
// - removeExpiredObjectsInCache no longer keeps a lock while the unref happens.
// - template allows customized KeyType.
// - objects with uninitialized time stamp are not removed.

/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
 */

#ifndef OPENMW_COMPONENTS_RESOURCE_OBJECTCACHE
#define OPENMW_COMPONENTS_RESOURCE_OBJECTCACHE

#include "cachestats.hpp"

#include <osg/Node>
#include <osg/ref_ptr>

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace osg
{
    class Object;
    class State;
    class NodeVisitor;
    class Stats;
}

namespace Resource
{
    // How the cache asks "is anything outside the cache still holding this?".
    //
    // Free overloads rather than a member, so the cache never has to name a base
    // class its values are expected to derive from. This is the extension point: a
    // value type the cache has not seen before is supported by adding an overload
    // here, not by changing the cache or the type. A second rendering backend would
    // add one for its own pointer, since scene-graph objects must be owned by the
    // pointer their library refs them with — a graph holds references to its own
    // children, so a second refcount over the same object would be a bug, not a
    // design choice.
    template <class T>
    long useCount(const std::shared_ptr<T>& value)
    {
        return value.use_count();
    }

    template <class T>
    long useCount(const osg::ref_ptr<T>& value)
    {
        // nullptr is a storable value, so this has to answer for it; shared_ptr's
        // use_count() already returns 0 for an empty pointer.
        return value == nullptr ? 0 : value->referenceCount();
    }

    // The two things a value type may or may not be able to do. Named, because an
    // inline requires-expression on the member declaration formats unreadably.
    template <class T>
    concept ReleasesGLObjects = requires(T value, osg::State* state)
    {
        value->releaseGLObjects(state);
    };

    template <class T>
    concept HoldsOsgObject = std::is_convertible_v<decltype(std::declval<T>().get()), osg::Object*>;

    template <class ValueType>
    struct GenericObjectCacheItem
    {
        ValueType mValue;
        double mLastUsage;
    };

    template <typename KeyType, typename ValueType>
    class GenericObjectCache
    {
    public:
        /*
         * @brief Updates usage timestamps and removes expired items
         *
         * Updates the lastUsage timestamp of cached non-nullptr items that have external references.
         * Initializes lastUsage timestamp for new items.
         * Removes items that haven't been referenced for longer than expiryDelay.
         *
         * \note
         * Last usage might be updated from other places so nullptr items
         * that are not referenced elsewhere are not always removed.
         *
         * @param referenceTime the timestamp indicating when the item was most recently used
         * @param expiryDelay the delay after which the cache entry for an item expires
         */
        void update(double referenceTime, double expiryDelay)
        {
            std::vector<ValueType> objectsToRemove;
            {
                const double expiryTime = referenceTime - expiryDelay;
                std::lock_guard<std::mutex> lock(mMutex);

                std::erase_if(mItems, [&](auto& v) {
                    Item& item = v.second;

                    // update last usage timestamp if item is being referenced externally
                    // or initialize if not set
                    if (useCount(item.mValue) > 1 || item.mLastUsage == 0)
                        item.mLastUsage = referenceTime;

                    // skip items that have been accessed since expiryTime
                    if (item.mLastUsage > expiryTime)
                        return false;

                    ++mExpired;

                    // just mark for removal here so objects can be removed in bulk outside the lock
                    if (item.mValue != nullptr)
                        objectsToRemove.push_back(std::move(item.mValue));

                    return true;
                });
            }
            // remove expired items from cache
            objectsToRemove.clear();
        }

        /** Remove all objects in the cache regardless of having external references or expiry times.*/
        void clear()
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mItems.clear();
        }

        /** Add a key,object,timestamp triple to the Registry::ObjectCache.*/
        template <class K>
        void addEntryToObjectCache(K&& key, ValueType object, double timestamp = 0.0)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            const auto it = mItems.find(key);
            if (it == mItems.end())
                mItems.emplace_hint(it, std::forward<K>(key), Item{ std::move(object), timestamp });
            else
                it->second = Item{ std::move(object), timestamp };
        }

        /** Remove Object from cache.*/
        void removeFromObjectCache(const auto& key)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            const auto itr = mItems.find(key);
            if (itr != mItems.end())
                mItems.erase(itr);
        }

        /** Get a value from the object cache; a default-constructed one if absent. */
        ValueType getRefFromObjectCache(const auto& key)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (Item* const item = find(key))
                return item->mValue;
            return ValueType();
        }

        std::optional<ValueType> getRefFromObjectCacheOrNone(const auto& key)
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            if (Item* const item = find(key))
                return item->mValue;
            return std::nullopt;
        }

        /** Check if an object is in the cache, and if it is, update its usage time stamp. */
        bool checkInObjectCache(const auto& key, double timeStamp)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (Item* const item = find(key))
            {
                item->mLastUsage = timeStamp;
                return true;
            }
            return false;
        }

        /** call releaseGLObjects on all objects attached to the object cache.
            Constrained, so instantiating the cache with a value type that has no such
            method is not an error — it simply does not offer this. */
        void releaseGLObjects(osg::State* state) requires ReleasesGLObjects<ValueType>
        {
            std::lock_guard<std::mutex> lock(mMutex);
            for (const auto& [k, v] : mItems)
                v.mValue->releaseGLObjects(state);
        }

        /** call node->accept(nv); for all nodes in the objectCache. Constrained for
            the same reason as releaseGLObjects. */
        void accept(osg::NodeVisitor& nv) requires HoldsOsgObject<ValueType>
        {
            std::lock_guard<std::mutex> lock(mMutex);
            for (const auto& [k, v] : mItems)
                if (osg::Object* const object = v.mValue.get())
                    if (osg::Node* const node = dynamic_cast<osg::Node*>(object))
                        node->accept(nv);
        }

        /** call operator()(KeyType, osg::Object*) for each object in the cache. */
        template <class Functor>
        void call(Functor&& f)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            for (const auto& [k, v] : mItems)
                f(k, v.mValue.get());
        }

        template <class K>
        std::optional<std::pair<KeyType, ValueType>> lowerBound(K&& key)
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            const auto it = mItems.lower_bound(std::forward<K>(key));
            if (it == mItems.end())
                return std::nullopt;
            return std::pair(it->first, it->second.mValue);
        }

        CacheStats getStats() const
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            return CacheStats{
                .mSize = mItems.size(),
                .mGet = mGet,
                .mHit = mHit,
                .mExpired = mExpired,
            };
        }

    protected:
        using Item = GenericObjectCacheItem<ValueType>;

        std::map<KeyType, Item, std::less<>> mItems;
        mutable std::mutex mMutex;
        std::size_t mGet = 0;
        std::size_t mHit = 0;
        std::size_t mExpired = 0;

        Item* find(const auto& key)
        {
            ++mGet;
            const auto it = mItems.find(key);
            if (it == mItems.end())
                return nullptr;
            ++mHit;
            return &it->second;
        }
    };
}

#endif
