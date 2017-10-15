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

#include "objectcache.hpp"

#include <osg/Object>
#include <osg/Node>

namespace Resource
{

////////////////////////////////////////////////////////////////////////////////////////////
//
// ObjectCache
//
ObjectCache::ObjectCache():
    osg::Referenced(true)
{
}

ObjectCache::~ObjectCache()
{
}

void ObjectCache::addEntryToObjectCache(const std::string& filename, osg::Object* object, double timestamp)
{
    if (!object)
    {
        OSG_ALWAYS << " trying to add NULL object to cache for " << filename << std::endl;
        return;
    }
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    _objectCache[filename]=ObjectTimeStampPair(object,timestamp);
}

osg::ref_ptr<osg::Object> ObjectCache::getRefFromObjectCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    ObjectCacheMap::iterator itr = _objectCache.find(fileName);
    if (itr!=_objectCache.end())
    {
        return itr->second.first;
    }
    else return 0;
}

bool ObjectCache::checkInObjectCache(const std::string &fileName, double timeStamp)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    ObjectCacheMap::iterator itr = _objectCache.find(fileName);
    if (itr!=_objectCache.end())
    {
        itr->second.second = timeStamp;
        return true;
    }
    else return false;
}

void ObjectCache::updateTimeStampOfObjectsInCacheWithExternalReferences(double referenceTime)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

    // look for objects with external references and update their time stamp.
    for(ObjectCacheMap::iterator itr=_objectCache.begin();
        itr!=_objectCache.end();
        ++itr)
    {
        // if ref count is greater the 1 the object has an external reference.
        if (itr->second.first->referenceCount()>1)
        {
            // so update it time stamp.
            itr->second.second = referenceTime;
        }
    }
}

void ObjectCache::removeExpiredObjectsInCache(double expiryTime)
{
    std::vector<osg::ref_ptr<osg::Object> > objectsToRemove;

    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

        // Remove expired entries from object cache
        ObjectCacheMap::iterator oitr = _objectCache.begin();
        while(oitr != _objectCache.end())
        {
            if (oitr->second.second<=expiryTime)
            {
                objectsToRemove.push_back(oitr->second.first);
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

void ObjectCache::removeFromObjectCache(const std::string& fileName)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    ObjectCacheMap::iterator itr = _objectCache.find(fileName);
    if (itr!=_objectCache.end()) _objectCache.erase(itr);
}

void ObjectCache::clear()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    _objectCache.clear();
}

void ObjectCache::releaseGLObjects(osg::State* state)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

    for(ObjectCacheMap::iterator itr = _objectCache.begin();
        itr != _objectCache.end();
        ++itr)
    {
        osg::Object* object = itr->second.first.get();
        object->releaseGLObjects(state);
    }
}

void ObjectCache::accept(osg::NodeVisitor &nv)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);

    for(ObjectCacheMap::iterator itr = _objectCache.begin();
        itr != _objectCache.end();
        ++itr)
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

unsigned int ObjectCache::getCacheSize() const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_objectCacheMutex);
    return _objectCache.size();
}

}
