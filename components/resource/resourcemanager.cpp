#include "resourcemanager.hpp"

#include "objectcache.hpp"

namespace Resource
{

    ResourceManager::ResourceManager(const VFS::Manager *vfs)
        : mVFS(vfs)
        , mCache(new Resource::ObjectCache)
        , mExpiryDelay(0.0)
    {

    }

    ResourceManager::~ResourceManager()
    {
    }

    void ResourceManager::updateCache(double referenceTime)
    {
        mCache->updateTimeStampOfObjectsInCacheWithExternalReferences(referenceTime);
        mCache->removeExpiredObjectsInCache(referenceTime - mExpiryDelay);
    }

    void ResourceManager::setExpiryDelay(double expiryDelay)
    {
        mExpiryDelay = expiryDelay;
    }

    const VFS::Manager* ResourceManager::getVFS() const
    {
        return mVFS;
    }

}
