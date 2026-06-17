#include "multiobjectcache.hpp"

#include <vector>

#include <osg/Object>

namespace Resource
{
    void MultiObjectCache::removeUnreferencedObjectsInCache()
    {
        std::vector<osg::ref_ptr<osg::Object>> objectsToRemove;
        {
            std::lock_guard<std::mutex> lock(_objectCacheMutex);

            // Remove unreferenced entries from object cache
            ObjectCacheMap::iterator oitr = _objectCache.begin();
            while (oitr != _objectCache.end())
            {
                if (oitr->second->referenceCount() <= 1)
                {
                    objectsToRemove.push_back(oitr->second);
                    _objectCache.erase(oitr++);
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
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        _objectCache.clear();
    }

    void MultiObjectCache::addEntryToObjectCache(VFS::Path::NormalizedView filename, osg::Object* object)
    {
        if (!object)
        {
            OSG_ALWAYS << " trying to add NULL object to cache for " << filename << std::endl;
            return;
        }
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        _objectCache.emplace(filename, object);
    }

    osg::ref_ptr<osg::Object> MultiObjectCache::takeFromObjectCache(VFS::Path::NormalizedView fileName)
    {
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        ++mGet;
        const auto it = _objectCache.find(fileName);
        if (it != _objectCache.end())
        {
            osg::ref_ptr<osg::Object> object = std::move(it->second);
            _objectCache.erase(it);
            ++mHit;
            return object;
        }

        return nullptr;
    }

    void MultiObjectCache::releaseGLObjects(osg::State* state)
    {
        std::lock_guard<std::mutex> lock(_objectCacheMutex);

        for (ObjectCacheMap::iterator itr = _objectCache.begin(); itr != _objectCache.end(); ++itr)
        {
            osg::Object* object = itr->second.get();
            object->releaseGLObjects(state);
        }
    }

    CacheStats MultiObjectCache::getStats() const
    {
        std::lock_guard<std::mutex> lock(_objectCacheMutex);
        return CacheStats{
            .mSize = _objectCache.size(),
            .mGet = mGet,
            .mHit = mHit,
            .mExpired = mExpired,
        };
    }

}
