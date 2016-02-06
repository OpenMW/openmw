#include "resourcemanager.hpp"

#include "objectcache.hpp"

namespace Resource
{

    ResourceManager::ResourceManager(const VFS::Manager *vfs, const double expiryDelay)
        : mVFS(vfs)
        , mCache(new Resource::ObjectCache)
        , mExpiryDelay(expiryDelay)
    {

    }

    void ResourceManager::updateCache(double referenceTime)
    {
        // NOTE: we could clear the cache from the background thread if the deletion proves too much of an overhead
        // idea: customize objectCache to not hold a lock while doing the actual deletion
        mCache->updateTimeStampOfObjectsInCacheWithExternalReferences(referenceTime);
        mCache->removeExpiredObjectsInCache(referenceTime - mExpiryDelay);
    }

    void ResourceManager::clearCache()
    {
        mCache->clear();
    }

    const VFS::Manager* ResourceManager::getVFS() const
    {
        return mVFS;
    }

}
