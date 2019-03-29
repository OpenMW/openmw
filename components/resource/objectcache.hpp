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

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <osg/Node>

#include <string>
#include <map>

namespace osg
{
    class Object;
    class State;
    class NodeVisitor;
}

namespace Resource {

template <typename KeyType>
class GenericObjectCache : public osg::Referenced
{
    public:

        GenericObjectCache()
            : osg::Referenced(true) {}

        /** For each object in the cache which has an reference count greater than 1
          * (and therefore referenced by elsewhere in the application) set the time stamp
          * for that object in the cache to specified time.
          * This would typically be called once per frame by applications which are doing database paging,
          * and need to prune objects that are no longer required.
          * The time used should be taken from the FrameStamp::getReferenceTime().*/
        void updateTimeStampOfObjectsInCacheWithExternalReferences(double referenceTime)
        {
            // look for objects with external references and update their time stamp.
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            for(typename ObjectCacheMap::iterator itr=_objectCache.begin(); itr!=_objectCache.end(); ++itr)
            {
                // If ref count is greater than 1, the object has an external reference.
                // If the timestamp is yet to be initialized, it needs to be updated too.
                if (itr->second.first->referenceCount()>1 || itr->second.second == 0.0)
                    itr->second.second = referenceTime;
            }
        }

        /** Removed object in the cache which have a time stamp at or before the specified expiry time.
          * This would typically be called once per frame by applications which are doing database paging,
          * and need to prune objects that are no longer required, and called after the a called
          * after the call to updateTimeStampOfObjectsInCacheWithExternalReferences(expirtyTime).*/
        void removeExpiredObjectsInCache(double expiryTime)
        {
            std::vector<osg::ref_ptr<osg::Object> > objectsToRemove;
            {
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
                // Remove expired entries from object cache
                typename ObjectCacheMap::iterator oitr = _objectCache.begin();
                while(oitr != _objectCache.end())
                {
                    if (oitr->second.second<=expiryTime)
                    {
                        objectsToRemove.push_back(oitr->second.first);
                        _objectCache.erase(oitr++);
                    }
                    else
                        ++oitr;
                }
            }
            // note, actual unref happens outside of the lock
            objectsToRemove.clear();
        }

        /** Remove all objects in the cache regardless of having external references or expiry times.*/
        void clear()
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            _objectCache.clear();
        }

        /** Add a key,object,timestamp triple to the Registry::ObjectCache.*/
        void addEntryToObjectCache(const KeyType& key, osg::Object* object, double timestamp = 0.0)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            _objectCache[key]=ObjectTimeStampPair(object,timestamp);
        }

        /** Remove Object from cache.*/
        void removeFromObjectCache(const KeyType& key)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr!=_objectCache.end()) _objectCache.erase(itr);
        }

        /** Get an ref_ptr<Object> from the object cache*/
        osg::ref_ptr<osg::Object> getRefFromObjectCache(const KeyType& key)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr!=_objectCache.end())
                return itr->second.first;
            else return 0;
        }

        /** Check if an object is in the cache, and if it is, update its usage time stamp. */
        bool checkInObjectCache(const KeyType& key, double timeStamp)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            typename ObjectCacheMap::iterator itr = _objectCache.find(key);
            if (itr!=_objectCache.end())
            {
                itr->second.second = timeStamp;
                return true;
            }
            else return false;
        }

        /** call releaseGLObjects on all objects attached to the object cache.*/
        void releaseGLObjects(osg::State* state)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            for(typename ObjectCacheMap::iterator itr = _objectCache.begin(); itr != _objectCache.end(); ++itr)
            {
                osg::Object* object = itr->second.first.get();
                object->releaseGLObjects(state);
            }
        }

        /** call node->accept(nv); for all nodes in the objectCache. */
        void accept(osg::NodeVisitor& nv)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            for(typename ObjectCacheMap::iterator itr = _objectCache.begin(); itr != _objectCache.end(); ++itr)
            {
                osg::Object* object = itr->second.first.get();
                if (object)
                {
                    osg::Node* node = dynamic_cast<osg::Node*>(object);
                    if (node)
                        node->accept(nv);
                }
            }
        }

        /** call operator()(osg::Object*) for each object in the cache. */
        template <class Functor>
        void call(Functor& f)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            for (typename ObjectCacheMap::iterator it = _objectCache.begin(); it != _objectCache.end(); ++it)
                f(it->second.first.get());
        }

        /** Get the number of objects in the cache. */
        unsigned int getCacheSize() const
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
            return _objectCache.size();
        }

    protected:

        virtual ~GenericObjectCache() {}

        typedef std::pair<osg::ref_ptr<osg::Object>, double >           ObjectTimeStampPair;
        typedef std::map<KeyType, ObjectTimeStampPair >             ObjectCacheMap;

        ObjectCacheMap                          _objectCache;
        mutable OpenThreads::Mutex              _objectCacheMutex;

};

class ObjectCache : public GenericObjectCache<std::string>
{
};

}

#endif
