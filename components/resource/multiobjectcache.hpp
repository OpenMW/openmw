#ifndef OPENMW_COMPONENTS_MULTIOBJECTCACHE_H
#define OPENMW_COMPONENTS_MULTIOBJECTCACHE_H

#include <map>
#include <mutex>
#include <string>

#include <osg/Referenced>
#include <osg/ref_ptr>

#include "cachestats.hpp"

namespace osg
{
    class Object;
    class State;
}

namespace Resource
{

    /// @brief Cache for "non reusable" objects.
    class MultiObjectCache : public osg::Referenced
    {
    public:
        MultiObjectCache();
        ~MultiObjectCache();

        void removeUnreferencedObjectsInCache();

        /** Remove all objects from the cache. */
        void clear();

        void addEntryToObjectCache(const std::string& filename, osg::Object* object);

        /** Take an Object from cache. Return nullptr if no object found. */
        osg::ref_ptr<osg::Object> takeFromObjectCache(const std::string& fileName);

        /** call releaseGLObjects on all objects attached to the object cache.*/
        void releaseGLObjects(osg::State* state);

        CacheStats getStats() const;

    protected:
        typedef std::multimap<std::string, osg::ref_ptr<osg::Object>> ObjectCacheMap;

        ObjectCacheMap _objectCache;
        mutable std::mutex _objectCacheMutex;
        std::size_t mGet = 0;
        std::size_t mHit = 0;
        std::size_t mExpired = 0;
    };

}

#endif
