#include "multiobjectcache.hpp"

#include <vector>

#include <osg/Object>

namespace Resource
{
    void MultiObjectCache::removeUnreferencedObjectsInCache()
    {
        std::vector<osg::ref_ptr<osg::Object>> objectsToRemove;
        {
            std::lock_guard<std::mutex> lock(mObjectCacheMutex);

            // Remove unreferenced entries from object cache
            ObjectCacheMap::iterator oitr = mObjectCache.begin();
            while (oitr != mObjectCache.end())
            {
                if (oitr->second->referenceCount() <= 1)
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

        // note, actual unref happens outside of the lock
        objectsToRemove.clear();
    }

    void MultiObjectCache::clear()
    {
        std::lock_guard<std::mutex> lock(mObjectCacheMutex);
        mObjectCache.clear();
    }

    void MultiObjectCache::addEntryToObjectCache(VFS::Path::NormalizedView filename, osg::Object* object)
    {
        if (!object)
        {
            OSG_ALWAYS << " trying to add NULL object to cache for " << filename << std::endl;
            return;
        }
        std::lock_guard<std::mutex> lock(mObjectCacheMutex);
        mObjectCache.emplace(filename, object);
    }

    osg::ref_ptr<osg::Object> MultiObjectCache::takeFromObjectCache(VFS::Path::NormalizedView fileName)
    {
        std::lock_guard<std::mutex> lock(mObjectCacheMutex);
        ++mGet;
        const auto it = mObjectCache.find(fileName);
        if (it != mObjectCache.end())
        {
            osg::ref_ptr<osg::Object> object = std::move(it->second);
            mObjectCache.erase(it);
            ++mHit;
            return object;
        }

        return nullptr;
    }

    void MultiObjectCache::releaseGLObjects(osg::State* state)
    {
        std::lock_guard<std::mutex> lock(mObjectCacheMutex);

        for (ObjectCacheMap::iterator itr = mObjectCache.begin(); itr != mObjectCache.end(); ++itr)
        {
            osg::Object* object = itr->second.get();
            object->releaseGLObjects(state);
        }
    }

    CacheStats MultiObjectCache::getStats() const
    {
        std::lock_guard<std::mutex> lock(mObjectCacheMutex);
        return CacheStats{
            .mSize = mObjectCache.size(),
            .mGet = mGet,
            .mHit = mHit,
            .mExpired = mExpired,
        };
    }

}
