// Resource ObjectCache for OpenMW, forked from osgDB ObjectCache by Robert Osfield, see copyright notice below.
// The main change from the upstream version is that removeExpiredObjectsInCache no longer keeps a lock while the unref happens.

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

#include <osg/Referenced>
#include <osg/ref_ptr>

#include <string>
#include <map>

namespace osg
{
    class Object;
    class State;
    class NodeVisitor;
}

namespace Resource {

class ObjectCache : public osg::Referenced
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

        /** Add a filename,object,timestamp triple to the Registry::ObjectCache.*/
        void addEntryToObjectCache(const std::string& filename, osg::Object* object, double timestamp = 0.0);

        /** Remove Object from cache.*/
        void removeFromObjectCache(const std::string& fileName);

        /** Get an ref_ptr<Object> from the object cache*/
        osg::ref_ptr<osg::Object> getRefFromObjectCache(const std::string& fileName);

        /** call releaseGLObjects on all objects attached to the object cache.*/
        void releaseGLObjects(osg::State* state);

        /** call node->accept(nv); for all nodes in the objectCache. */
        void accept(osg::NodeVisitor& nv);

    protected:

        virtual ~ObjectCache();

        typedef std::pair<osg::ref_ptr<osg::Object>, double >           ObjectTimeStampPair;
        typedef std::map<std::string, ObjectTimeStampPair >             ObjectCacheMap;

        ObjectCacheMap                          _objectCache;
        OpenThreads::Mutex                      _objectCacheMutex;

};

}

#endif
