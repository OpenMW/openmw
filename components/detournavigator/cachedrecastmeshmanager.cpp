#include "cachedrecastmeshmanager.hpp"
#include "debug.hpp"

namespace DetourNavigator
{
    CachedRecastMeshManager::CachedRecastMeshManager(const Settings& settings)
        : mImpl(settings)
    {
    }

    bool CachedRecastMeshManager::removeObject(std::size_t id)
    {
        if (!mImpl.removeObject(id))
            return false;
        mCached.reset();
        return true;
    }

    std::shared_ptr<RecastMesh> CachedRecastMeshManager::getMesh()
    {
        if (!mCached)
            mCached = mImpl.getMesh();
        return mCached;
    }
}
