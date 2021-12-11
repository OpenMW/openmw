#include "recastmeshmanager.hpp"
#include "recastmeshbuilder.hpp"
#include "settings.hpp"
#include "heightfieldshape.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>

#include <utility>

namespace
{
    struct AddHeightfield
    {
        osg::Vec2i mCellPosition;
        int mCellSize;
        DetourNavigator::RecastMeshBuilder& mBuilder;

        void operator()(const DetourNavigator::HeightfieldSurface& v)
        {
            mBuilder.addHeightfield(mCellPosition, mCellSize, v.mHeights, v.mSize, v.mMinHeight, v.mMaxHeight);
        }

        void operator()(DetourNavigator::HeightfieldPlane v)
        {
            mBuilder.addHeightfield(mCellPosition, mCellSize, v.mHeight);
        }
    };
}

namespace DetourNavigator
{
    RecastMeshManager::RecastMeshManager(const TileBounds& bounds, std::size_t generation)
        : mGeneration(generation)
        , mTileBounds(bounds)
    {
    }

    bool RecastMeshManager::addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        const std::lock_guard lock(mMutex);
        const auto object = mObjects.lower_bound(id);
        if (object != mObjects.end() && object->first == id)
            return false;
        mObjects.emplace_hint(object, id,
            OscillatingRecastMeshObject(RecastMeshObject(shape, transform, areaType), mRevision + 1));
        ++mRevision;
        return true;
    }

    bool RecastMeshManager::updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType)
    {
        const std::lock_guard lock(mMutex);
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return false;
        const std::size_t lastChangeRevision = mLastNavMeshReportedChange.has_value()
                ? mLastNavMeshReportedChange->mRevision : mRevision;
        if (!object->second.update(transform, areaType, lastChangeRevision, mTileBounds))
            return false;
        ++mRevision;
        return true;
    }

    std::optional<RemovedRecastMeshObject> RecastMeshManager::removeObject(const ObjectId id)
    {
        const std::lock_guard lock(mMutex);
        const auto object = mObjects.find(id);
        if (object == mObjects.end())
            return std::nullopt;
        const RemovedRecastMeshObject result {object->second.getImpl().getShape(), object->second.getImpl().getTransform()};
        mObjects.erase(object);
        ++mRevision;
        return result;
    }

    bool RecastMeshManager::addWater(const osg::Vec2i& cellPosition, int cellSize, float level)
    {
        const std::lock_guard lock(mMutex);
        if (!mWater.emplace(cellPosition, Water {cellSize, level}).second)
            return false;
        ++mRevision;
        return true;
    }

    std::optional<Water> RecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const std::lock_guard lock(mMutex);
        const auto water = mWater.find(cellPosition);
        if (water == mWater.end())
            return std::nullopt;
        ++mRevision;
        Water result = water->second;
        mWater.erase(water);
        return result;
    }

    bool RecastMeshManager::addHeightfield(const osg::Vec2i& cellPosition, int cellSize,
        const HeightfieldShape& shape)
    {
        const std::lock_guard lock(mMutex);
        if (!mHeightfields.emplace(cellPosition, SizedHeightfieldShape {cellSize, shape}).second)
            return false;
        ++mRevision;
        return true;
    }

    std::optional<SizedHeightfieldShape> RecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const std::lock_guard lock(mMutex);
        const auto it = mHeightfields.find(cellPosition);
        if (it == mHeightfields.end())
            return std::nullopt;
        ++mRevision;
        auto result = std::make_optional(it->second);
        mHeightfields.erase(it);
        return result;
    }

    std::shared_ptr<RecastMesh> RecastMeshManager::getMesh() const
    {
        RecastMeshBuilder builder(mTileBounds);
        using Object = std::tuple<
            osg::ref_ptr<const Resource::BulletShapeInstance>,
            ObjectTransform,
            std::reference_wrapper<const btCollisionShape>,
            btTransform,
            AreaType
        >;
        std::vector<Object> objects;
        std::size_t revision;
        {
            const std::lock_guard lock(mMutex);
            for (const auto& [k, v] : mWater)
                builder.addWater(k, v);
            for (const auto& [cellPosition, v] : mHeightfields)
                std::visit(AddHeightfield {cellPosition, v.mCellSize, builder}, v.mShape);
            objects.reserve(mObjects.size());
            for (const auto& [k, object] : mObjects)
            {
                const RecastMeshObject& impl = object.getImpl();
                objects.emplace_back(impl.getInstance(), impl.getObjectTransform(), impl.getShape(),
                                     impl.getTransform(), impl.getAreaType());
            }
            revision = mRevision;
        }
        for (const auto& [instance, objectTransform, shape, transform, areaType] : objects)
            builder.addObject(shape, transform, areaType, instance->getSource(), objectTransform);
        return std::move(builder).create(mGeneration, revision);
    }

    bool RecastMeshManager::isEmpty() const
    {
        const std::lock_guard lock(mMutex);
        return mObjects.empty() && mWater.empty() && mHeightfields.empty();
    }

    void RecastMeshManager::reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion)
    {
        if (recastMeshVersion.mGeneration != mGeneration)
            return;
        const std::lock_guard lock(mMutex);
        if (mLastNavMeshReport.has_value() && navMeshVersion < mLastNavMeshReport->mNavMeshVersion)
            return;
        mLastNavMeshReport = {recastMeshVersion.mRevision, navMeshVersion};
        if (!mLastNavMeshReportedChange.has_value()
                || mLastNavMeshReportedChange->mNavMeshVersion < mLastNavMeshReport->mNavMeshVersion)
            mLastNavMeshReportedChange = mLastNavMeshReport;
    }

    Version RecastMeshManager::getVersion() const
    {
        const std::lock_guard lock(mMutex);
        return Version {mGeneration, mRevision};
    }
}
