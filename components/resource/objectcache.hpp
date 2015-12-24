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

// Wrapper for osgDB/ObjectCache. Works around ObjectCache not being available in old OSG 3.2.
// Use "#include objectcache.hpp" in place of "#include <osgDB/ObjectCache".

#ifndef OSGDB_OBJECTCACHE_WRAPPER
#define OSGDB_OBJECTCACHE_WRAPPER 1

#include <osg/Version>

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
#include <osgDB/ObjectCache>
#else

#include <osg/Node>

#include <osgDB/ReaderWriter>
#include <osgDB/DatabaseRevisions>

#include <map>

namespace osgDB {

class /*OSGDB_EXPORT*/ ObjectCache : public osg::Referenced
{
    public:

        ObjectCache();

        /** For each object in the cache which has an reference count greater than 1
          * (and therefore referenced by elsewhere in the application) set the time stamp
          * for that object in the cache to specified time.
          * This would typically be called once per frame by applications which are doing database paging,
          * and need to prune objects that are no longer required.
          * The time used should be taken from the FrameStamp::getReferenceTime().*/
        void updateTimeStampOfObjectsInCacheWithExternalReferences(double referenceTime);

        /** Removed object in the cache which have a time stamp at or before the specified expiry time.
          * This would typically be called once per frame by applications which are doing database paging,
          * and need to prune objects that are no longer required, and called after the a called
          * after the call to updateTimeStampOfObjectsInCacheWithExternalReferences(expirtyTime).*/
        void removeExpiredObjectsInCache(double expiryTime);

        /** Remove all objects in the cache regardless of having external references or expiry times.*/
        void clear();

        /** Add contents of specified ObjectCache to this object cache.*/
        void addObjectCache(ObjectCache* object);

        /** Add a filename,object,timestamp triple to the Registry::ObjectCache.*/
        void addEntryToObjectCache(const std::string& filename, osg::Object* object, double timestamp = 0.0);

        /** Remove Object from cache.*/
        void removeFromObjectCache(const std::string& fileName);

        /** Get an Object from the object cache*/
        osg::Object* getFromObjectCache(const std::string& fileName);

        /** Get an ref_ptr<Object> from the object cache*/
        osg::ref_ptr<osg::Object> getRefFromObjectCache(const std::string& fileName);

        /** call rleaseGLObjects on all objects attached to the object cache.*/
        void releaseGLObjects(osg::State* state);

    protected:

        virtual ~ObjectCache();

        typedef std::pair<osg::ref_ptr<osg::Object>, double >           ObjectTimeStampPair;
        typedef std::map<std::string, ObjectTimeStampPair >             ObjectCacheMap;

        ObjectCacheMap                          _objectCache;
        OpenThreads::Mutex                      _objectCacheMutex;

};

}

#endif

#endif
