#include "recastmeshmanager.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace DetourNavigator
{
    RecastMeshManager::RecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation)
        : mGeneration(generation)
        , mMeshBuilder(settings, bounds)
    {
    }

    bool RecastMeshManager::addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        const auto iterator = mObjectsOrder.emplace(mObjectsOrder.end(), RecastMeshObject(shape, transform, areaType));
        if (!mObjects.emplace(id, iterator).second)
        {
            mObjectsOrder.erase(iterator);
            return false;
        }
        ++mRevision;
        return true;
    }

    bool RecastMeshManager::updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return false;
        if (!object->second->update(transform, areaType))
            return false;
        ++mRevision;
        return true;
    }

    boost::optional<RemovedRecastMeshObject> RecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return boost::none;
        const RemovedRecastMeshObject result {object->second->getShape(), object->second->getTransform()};
        mObjectsOrder.erase(object->second);
        mObjects.erase(object);
        ++mRevision;
        return result;
    }

    bool RecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize,
        const btTransform& transform)
    {
        const auto iterator = mWaterOrder.emplace(mWaterOrder.end(), Water {cellSize, transform});
        if (!mWater.emplace(cellPosition, iterator).second)
        {
            mWaterOrder.erase(iterator);
            return false;
        }
        ++mRevision;
        return true;
    }

    boost::optional<RecastMeshManager::Water> RecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto water = mWater.find(cellPosition);
        if (water == mWater.end())
            return boost::none;
        ++mRevision;
        const auto result = *water->second;
        mWaterOrder.erase(water->second);
        mWater.erase(water);
        return result;
    }

    std::shared_ptr<RecastMesh> RecastMeshManager::getMesh()
    {
        rebuild();
        return mMeshBuilder.create(mGeneration, mLastBuildRevision);
    }

    bool RecastMeshManager::isEmpty() const
    {
        return mObjects.empty();
    }

    void RecastMeshManager::rebuild()
    {
        if (mLastBuildRevision == mRevision)
            return;
        mMeshBuilder.reset();
        for (const auto& v : mWaterOrder)
            mMeshBuilder.addWater(v.mCellSize, v.mTransform);
        for (const auto& v : mObjectsOrder)
            mMeshBuilder.addObject(v.getShape(), v.getTransform(), v.getAreaType());
        mLastBuildRevision = mRevision;
    }
}
