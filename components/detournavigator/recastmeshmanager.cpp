#include "recastmeshmanager.hpp"

namespace DetourNavigator
{
    RecastMeshManager::RecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation)
        : mGeneration(generation)
        , mMeshBuilder(settings, bounds)
        , mTileBounds(bounds)
    {
    }

    bool RecastMeshManager::addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        const auto object = mObjects.lower_bound(id);
        if (object != mObjects.end() && object->first == id)
            return false;
        const auto iterator = mObjectsOrder.emplace(mObjectsOrder.end(),
            OscillatingRecastMeshObject(RecastMeshObject(shape, transform, areaType), mRevision + 1));
        mObjects.emplace_hint(object, id, iterator);
        ++mRevision;
        return true;
    }

    bool RecastMeshManager::updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return false;
        const std::size_t lastChangeRevision = mLastNavMeshReportedChange.has_value()
                ? mLastNavMeshReportedChange->mRevision : mRevision;
        if (!object->second->update(transform, areaType, lastChangeRevision, mTileBounds))
            return false;
        ++mRevision;
        return true;
    }

    std::optional<RemovedRecastMeshObject> RecastMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return std::nullopt;
        const RemovedRecastMeshObject result {object->second->getImpl().getShape(), object->second->getImpl().getTransform()};
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

    std::optional<RecastMeshManager::Water> RecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto water = mWater.find(cellPosition);
        if (water == mWater.end())
            return std::nullopt;
        ++mRevision;
        const auto result = *water->second;
        mWaterOrder.erase(water->second);
        mWater.erase(water);
        return result;
    }

    std::shared_ptr<RecastMesh> RecastMeshManager::getMesh()
    {
        rebuild();
        return mMeshBuilder.create(mGeneration, mRevision);
    }

    bool RecastMeshManager::isEmpty() const
    {
        return mObjects.empty();
    }

    void RecastMeshManager::reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion)
    {
        if (recastMeshVersion.mGeneration != mGeneration)
            return;
        if (mLastNavMeshReport.has_value() && navMeshVersion < mLastNavMeshReport->mNavMeshVersion)
            return;
        mLastNavMeshReport = {recastMeshVersion.mRevision, navMeshVersion};
        if (!mLastNavMeshReportedChange.has_value()
                || mLastNavMeshReportedChange->mNavMeshVersion < mLastNavMeshReport->mNavMeshVersion)
            mLastNavMeshReportedChange = mLastNavMeshReport;
    }

    Version RecastMeshManager::getVersion() const
    {
        return Version {mGeneration, mRevision};
    }

    void RecastMeshManager::rebuild()
    {
        mMeshBuilder.reset();
        for (const auto& v : mWaterOrder)
            mMeshBuilder.addWater(v.mCellSize, v.mTransform);
        for (const auto& object : mObjectsOrder)
        {
            const RecastMeshObject& v = object.getImpl();
            mMeshBuilder.addObject(v.getShape(), v.getTransform(), v.getAreaType());
        }
    }
}
