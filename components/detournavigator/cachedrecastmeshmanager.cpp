#include "cachedrecastmeshmanager.hpp"
#include "debug.hpp"

namespace DetourNavigator
{
    CachedRecastMeshManager::CachedRecastMeshManager(const Settings& settings, const TileBounds& bounds,
            std::size_t generation)
        : mImpl(settings, bounds, generation)
    {}

    bool CachedRecastMeshManager::addObject(const ObjectId id, const btCollisionShape& shape,
                                            const btTransform& transform, const AreaType areaType)
    {
        if (!mImpl.addObject(id, shape, transform, areaType))
            return false;
        mCached.reset();
        return true;
    }

    bool CachedRecastMeshManager::updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType)
    {
        if (!mImpl.updateObject(id, transform, areaType))
            return false;
        mCached.reset();
        return true;
    }

    std::optional<RemovedRecastMeshObject> CachedRecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mImpl.removeObject(id);
        if (object)
            mCached.reset();
        return object;
    }

    bool CachedRecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize,
        const osg::Vec3f& shift)
    {
        if (!mImpl.addWater(cellPosition, cellSize, shift))
            return false;
        mCached.reset();
        return true;
    }

    std::optional<Cell> CachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto water = mImpl.removeWater(cellPosition);
        if (water)
            mCached.reset();
        return water;
    }

    bool CachedRecastMeshManager::addHeightfield(const osg::Vec2i& cellPosition, int cellSize,
        const osg::Vec3f& shift, const HeightfieldShape& shape)
    {
        if (!mImpl.addHeightfield(cellPosition, cellSize, shift, shape))
            return false;
        mCached.reset();
        return true;
    }

    std::optional<Cell> CachedRecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const auto cell = mImpl.removeHeightfield(cellPosition);
        if (cell)
            mCached.reset();
        return cell;
    }

    std::shared_ptr<RecastMesh> CachedRecastMeshManager::getMesh()
    {
        if (!mCached)
            mCached = mImpl.getMesh();
        return mCached;
    }

    bool CachedRecastMeshManager::isEmpty() const
    {
        return mImpl.isEmpty();
    }

    void CachedRecastMeshManager::reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion)
    {
        mImpl.reportNavMeshChange(recastMeshVersion, navMeshVersion);
    }

    Version CachedRecastMeshManager::getVersion() const
    {
        return mImpl.getVersion();
    }
}
