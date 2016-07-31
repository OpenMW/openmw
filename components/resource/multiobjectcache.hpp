#ifndef OPENMW_COMPONENTS_MULTIOBJECTCACHE_H
#define OPENMW_COMPONENTS_MULTIOBJECTCACHE_H

#include <map>
#include <string>

#include <osg/ref_ptr>
#include <osg/Referenced>

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

        void addEntryToObjectCache(const std::string& filename, osg::Object* object);

        /** Take an Object from cache. Return NULL if no object found. */
        osg::ref_ptr<osg::Object> takeFromObjectCache(const std::string& fileName);

        /** call releaseGLObjects on all objects attached to the object cache.*/
        void releaseGLObjects(osg::State* state);

    protected:

        typedef std::multimap<std::string, osg::ref_ptr<osg::Object> >             ObjectCacheMap;

        ObjectCacheMap                          _objectCache;
        OpenThreads::Mutex                      _objectCacheMutex;

    };

}

#endif
