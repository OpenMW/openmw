#ifndef OPENMW_COMPONENTS_MULTIOBJECTCACHE_H
#define OPENMW_COMPONENTS_MULTIOBJECTCACHE_H

#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/vfs/pathutil.hpp>

#include "cachestats.hpp"
#include "objectcache.hpp"

namespace osg
{
    class State;
}

namespace Resource
{

    /// @brief Cache for "non reusable" objects. Templated on its value type for the
    /// same reason GenericObjectCache is; see objectcache.hpp.
    template <class ValueType>
    class MultiObjectCache
    {
    public:
        void removeUnreferencedObjectsInCache()
        {
            std::vector<ValueType> objectsToRemove;
            {
                std::lock_guard<std::mutex> lock(mObjectCacheMutex);

                auto oitr = mObjectCache.begin();
                while (oitr != mObjectCache.end())
                {
                    if (useCount(oitr->second) <= 1)
                    {
                        objectsToRemove.push_back(oitr->second);
                        mObjectCache.erase(oitr++);
                        ++mExpired;
                    }
                    else
                    {
                        ++oitr;
                    }
                }
            }

            // note, actual release happens outside of the lock
            objectsToRemove.clear();
        }

        /** Remove all objects from the cache. */
        void clear()
        {
            std::lock_guard<std::mutex> lock(mObjectCacheMutex);
            mObjectCache.clear();
        }

        void addEntryToObjectCache(VFS::Path::NormalizedView filename, ValueType object)
        {
            if (object == nullptr)
            {
                Log(Debug::Warning) << "Trying to add NULL object to cache for " << filename;
                return;
            }
            std::lock_guard<std::mutex> lock(mObjectCacheMutex);
            mObjectCache.emplace(filename, std::move(object));
        }

        /** Take an object from cache. Return nullptr if no object found. */
        ValueType takeFromObjectCache(VFS::Path::NormalizedView fileName)
        {
            std::lock_guard<std::mutex> lock(mObjectCacheMutex);
            ++mGet;
            const auto it = mObjectCache.find(fileName);
            if (it != mObjectCache.end())
            {
                ValueType object = std::move(it->second);
                mObjectCache.erase(it);
                ++mHit;
                return object;
            }

            return ValueType();
        }

        /** call releaseGLObjects on all objects attached to the object cache. */
        void releaseGLObjects(osg::State* state) requires ReleasesGLObjects<ValueType>
        {
            std::lock_guard<std::mutex> lock(mObjectCacheMutex);
            for (const auto& [key, value] : mObjectCache)
                value->releaseGLObjects(state);
        }

        CacheStats getStats() const
        {
            std::lock_guard<std::mutex> lock(mObjectCacheMutex);
            return CacheStats{
                .mSize = mObjectCache.size(),
                .mGet = mGet,
                .mHit = mHit,
                .mExpired = mExpired,
            };
        }

    protected:
        using ObjectCacheMap = std::multimap<VFS::Path::Normalized, ValueType, std::less<>>;

        ObjectCacheMap mObjectCache;
        mutable std::mutex mObjectCacheMutex;
        std::size_t mGet = 0;
        std::size_t mHit = 0;
        std::size_t mExpired = 0;
    };
}

#endif
