#ifndef OPENMW_COMPONENTS_RESOURCE_MANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_MANAGER_H

#include <osg/ref_ptr>

#include "objectcache.hpp"

namespace VFS
{
    class Manager;
}

namespace osg
{
    class Stats;
    class State;
}

namespace Resource
{

    class BaseResourceManager
    {
    public:
        virtual ~BaseResourceManager() {}
        virtual void updateCache(double referenceTime) {}
        virtual void clearCache() {}
        virtual void setExpiryDelay(double expiryDelay) {}
        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const {}
        virtual void releaseGLObjects(osg::State* state) {}
    };

    /// @brief Base class for managers that require a virtual file system and object cache.
    /// @par This base class implements clearing of the cache, but populating it and what it's used for is up to the individual sub classes.
    template <class KeyType>
    class GenericResourceManager : public BaseResourceManager
    {
    public:
        typedef GenericObjectCache<KeyType> CacheType;

        GenericResourceManager(const VFS::Manager* vfs)
            : mVFS(vfs)
            , mCache(new CacheType)
            , mExpiryDelay(0.0)
        {
        }

        virtual ~GenericResourceManager() {}

        /// Clear cache entries that have not been referenced for longer than expiryDelay.
        virtual void updateCache(double referenceTime)
        {
            mCache->updateTimeStampOfObjectsInCacheWithExternalReferences(referenceTime);
            mCache->removeExpiredObjectsInCache(referenceTime - mExpiryDelay);
        }

        /// Clear all cache entries.
        virtual void clearCache() { mCache->clear(); }

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay (double expiryDelay) { mExpiryDelay = expiryDelay; }

        const VFS::Manager* getVFS() const { return mVFS; }

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const {}

        virtual void releaseGLObjects(osg::State* state) { mCache->releaseGLObjects(state); }

    protected:
        const VFS::Manager* mVFS;
        osg::ref_ptr<CacheType> mCache;
        double mExpiryDelay;
    };


    class ResourceManager : public GenericResourceManager<std::string>
    {
    public:
        ResourceManager(const VFS::Manager* vfs) : GenericResourceManager<std::string>(vfs) {}
    };

}

#endif
