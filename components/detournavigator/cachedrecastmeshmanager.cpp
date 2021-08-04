#include "cachedrecastmeshmanager.hpp"
#include "debug.hpp"

namespace DetourNavigator
{
    CachedRecastMeshManager::CachedRecastMeshManager(const Settings& settings, const TileBounds& bounds,
            std::size_t generation)
        : mImpl(settings, bounds, generation)
    {}

    bool CachedRecastMeshManager::addObject(const ObjectId id, const CollisionShape& shape,
                                            const btTransform& transform, const AreaType areaType)
    {
        if (!mImpl.addObject(id, shape, transform, areaType))
            return false;
        mCached.lock()->reset();
        return true;
    }

    bool CachedRecastMeshManager::updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType)
    {
        if (!mImpl.updateObject(id, transform, areaType))
            return false;
        mCached.lock()->reset();
        return true;
    }

    std::optional<RemovedRecastMeshObject> CachedRecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mImpl.removeObject(id);
        if (object)
            mCached.lock()->reset();
        return object;
    }

    bool CachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize,
        const btTransform& transform)
    {
        if (!mImpl.addWater(cellPosition, cellSize, transform))
            return false;
        mCached.lock()->reset();
        return true;
    }

    std::optional<RecastMeshManager::Water> CachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto water = mImpl.removeWater(cellPosition);
        if (water)
            mCached.lock()->reset();
        return water;
    }

    std::shared_ptr<RecastMesh> CachedRecastMeshManager::getMesh()
    {
        std::shared_ptr<RecastMesh> cached = *mCached.lock();
        if (cached != nullptr)
            return cached;
        cached = mImpl.getMesh();
        *mCached.lock() = cached;
        return cached;
    }

    bool CachedRecastMeshManager::isEmpty() const
    {
        return mImpl.isEmpty();
    }

    void CachedRecastMeshManager::reportNavMeshChange(Version recastMeshVersion, Version navMeshVersion)
    {
        mImpl.reportNavMeshChange(recastMeshVersion, navMeshVersion);
    }

    Version CachedRecastMeshManager::getVersion() const
    {
        return mImpl.getVersion();
    }
}
