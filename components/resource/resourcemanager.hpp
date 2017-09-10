#ifndef OPENMW_COMPONENTS_RESOURCE_MANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_MANAGER_H

#include <osg/ref_ptr>

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
    class ObjectCache;

    /// @brief Base class for managers that require a virtual file system and object cache.
    /// @par This base class implements clearing of the cache, but populating it and what it's used for is up to the individual sub classes.
    class ResourceManager
    {
    public:
        ResourceManager(const VFS::Manager* vfs);
        virtual ~ResourceManager();

        /// Clear cache entries that have not been referenced for longer than expiryDelay.
        virtual void updateCache(double referenceTime);

        /// Clear all cache entries.
        virtual void clearCache();

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay (double expiryDelay);

        const VFS::Manager* getVFS() const;

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const {}

        virtual void releaseGLObjects(osg::State* state);

    protected:
        const VFS::Manager* mVFS;
        osg::ref_ptr<Resource::ObjectCache> mCache;
        double mExpiryDelay;
    };

}

#endif
