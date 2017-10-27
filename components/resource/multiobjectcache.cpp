#include "multiobjectcache.hpp"

#include <vector>

#include <osg/Object>

namespace Resource
{

    MultiObjectCache::MultiObjectCache()
    {

    }

    MultiObjectCache::~MultiObjectCache()
    {

    }

    void MultiObjectCache::removeUnreferencedObjectsInCache()
    {
        std::vector<osg::ref_ptr<osg::Object> > objectsToRemove;
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

            // Remove unreferenced entries from object cache
            ObjectCacheMap::iterator oitr = _objectCache.begin();
            while(oitr != _objectCache.end())
            {
                if (oitr->second->referenceCount() <= 1)
                {
                    objectsToRemove.push_back(oitr->second);
                    _objectCache.erase(oitr++);
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
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
        _objectCache.clear();
    }

    void MultiObjectCache::addEntryToObjectCache(const std::string &filename, osg::Object *object)
    {
        if (!object)
        {
            OSG_ALWAYS << " trying to add NULL object to cache for " << filename << std::endl;
            return;
        }
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
        _objectCache.insert(std::make_pair(filename, object));
    }

    osg::ref_ptr<osg::Object> MultiObjectCache::takeFromObjectCache(const std::string &fileName)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
        ObjectCacheMap::iterator found = _objectCache.find(fileName);
        if (found == _objectCache.end())
            return osg::ref_ptr<osg::Object>();
        else
        {
            osg::ref_ptr<osg::Object> object = found->second;
            _objectCache.erase(found);
            return object;
        }
    }

    void MultiObjectCache::releaseGLObjects(osg::State *state)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

        for(ObjectCacheMap::iterator itr = _objectCache.begin();
            itr != _objectCache.end();
            ++itr)
        {
            osg::Object* object = itr->second.get();
            object->releaseGLObjects(state);
        }
    }

    unsigned int MultiObjectCache::getCacheSize() const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
        return _objectCache.size();
    }

}
