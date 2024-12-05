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
#include <osg/Referenced>
#include <osg/ref_ptr>

#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <string>
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
    struct GenericObjectCacheItem
    {
        osg::ref_ptr<osg::Object> mValue;
        double mLastUsage;
    };

    template <typename KeyType>
    class GenericObjectCache : public osg::Referenced
    {
    public:
        // Update last usage timestamp using referenceTime for each cache time if they are not nullptr and referenced
        // from somewhere else. Remove items with last usage > expiryTime. Note: last usage might be updated from other
        // places so nullptr or not references elsewhere items are not always removed.
        void update(double referenceTime, double expiryDelay)
        {
            std::vector<osg::ref_ptr<osg::Object>> objectsToRemove;
            {
                const double expiryTime = referenceTime - expiryDelay;
                std::lock_guard<std::mutex> lock(mMutex);
                std::erase_if(mItems, [&](auto& v) {
                    Item& item = v.second;
                    if ((item.mValue != nullptr && item.mValue->referenceCount() > 1) || item.mLastUsage == 0)
                        item.mLastUsage = referenceTime;
                    if (item.mLastUsage > expiryTime)
                        return false;
                    ++mExpired;
                    if (item.mValue != nullptr)
                        objectsToRemove.push_back(std::move(item.mValue));
                    return true;
                });
            }
            // note, actual unref happens outside of the lock
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
        void addEntryToObjectCache(K&& key, osg::Object* object, double timestamp = 0.0)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            const auto it = mItems.find(key);
            if (it == mItems.end())
                mItems.emplace_hint(it, std::forward<K>(key), Item{ object, timestamp });
            else
                it->second = Item{ object, timestamp };
        }

        /** Remove Object from cache.*/
        void removeFromObjectCache(const auto& key)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            const auto itr = mItems.find(key);
            if (itr != mItems.end())
                mItems.erase(itr);
        }

        /** Get an ref_ptr<Object> from the object cache*/
        osg::ref_ptr<osg::Object> getRefFromObjectCache(const auto& key)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (Item* const item = find(key))
                return item->mValue;
            return nullptr;
        }

        std::optional<osg::ref_ptr<osg::Object>> getRefFromObjectCacheOrNone(const auto& key)
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

        /** call releaseGLObjects on all objects attached to the object cache.*/
        void releaseGLObjects(osg::State* state)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            for (const auto& [k, v] : mItems)
                v.mValue->releaseGLObjects(state);
        }

        /** call node->accept(nv); for all nodes in the objectCache. */
        void accept(osg::NodeVisitor& nv)
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
        std::optional<std::pair<KeyType, osg::ref_ptr<osg::Object>>> lowerBound(K&& key)
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
        using Item = GenericObjectCacheItem;

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
