#include "recastmeshmanager.hpp"
#include "recastmeshbuilder.hpp"
#include "settings.hpp"
#include "heightfieldshape.hpp"

#include <components/debug/debuglog.hpp>

#include <utility>

namespace
{
    struct AddHeightfield
    {
        const DetourNavigator::Cell& mCell;
        DetourNavigator::RecastMeshBuilder& mBuilder;

        void operator()(const DetourNavigator::HeightfieldSurface& v)
        {
            mBuilder.addHeightfield(mCell.mSize, mCell.mShift, v.mHeights, v.mSize, v.mMinHeight, v.mMaxHeight);
        }

        void operator()(DetourNavigator::HeightfieldPlane v)
        {
            mBuilder.addHeightfield(mCell.mSize, mCell.mShift, v.mHeight);
        }
    };
}

namespace DetourNavigator
{
    RecastMeshManager::RecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation)
        : mSettings(settings)
        , mGeneration(generation)
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

    bool RecastMeshManager::addWater(const osg::Vec2i& cellPosition, const int cellSize, const osg::Vec3f& shift)
    {
        const std::lock_guard lock(mMutex);
        if (!mWater.emplace(cellPosition, Cell {cellSize, shift}).second)
            return false;
        ++mRevision;
        return true;
    }

    std::optional<Cell> RecastMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const std::lock_guard lock(mMutex);
        const auto water = mWater.find(cellPosition);
        if (water == mWater.end())
            return std::nullopt;
        ++mRevision;
        const Cell result = water->second;
        mWater.erase(water);
        return result;
    }

    bool RecastMeshManager::addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const osg::Vec3f& shift,
        const HeightfieldShape& shape)
    {
        const std::lock_guard lock(mMutex);
        if (!mHeightfields.emplace(cellPosition, Heightfield {Cell {cellSize, shift}, shape}).second)
            return false;
        ++mRevision;
        return true;
    }

    std::optional<Cell> RecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const std::lock_guard lock(mMutex);
        const auto it = mHeightfields.find(cellPosition);
        if (it == mHeightfields.end())
            return std::nullopt;
        ++mRevision;
        const auto result = std::make_optional(it->second.mCell);
        mHeightfields.erase(it);
        return result;
    }

    std::shared_ptr<RecastMesh> RecastMeshManager::getMesh() const
    {
        TileBounds tileBounds = mTileBounds;
        tileBounds.mMin /= mSettings.mRecastScaleFactor;
        tileBounds.mMax /= mSettings.mRecastScaleFactor;
        RecastMeshBuilder builder(tileBounds);
        using Object = std::tuple<
            osg::ref_ptr<const osg::Referenced>,
            std::reference_wrapper<const btCollisionShape>,
            btTransform,
            AreaType
        >;
        std::vector<Object> objects;
        std::size_t revision;
        {
            const std::lock_guard lock(mMutex);
            for (const auto& [k, v] : mWater)
                builder.addWater(v.mSize, v.mShift);
            for (const auto& [cellPosition, v] : mHeightfields)
                std::visit(AddHeightfield {v.mCell, builder}, v.mShape);
            objects.reserve(mObjects.size());
            for (const auto& [k, object] : mObjects)
            {
                const RecastMeshObject& impl = object.getImpl();
                objects.emplace_back(impl.getHolder(), impl.getShape(), impl.getTransform(), impl.getAreaType());
            }
            revision = mRevision;
        }
        for (const auto& [holder, shape, transform, areaType] : objects)
            builder.addObject(shape, transform, areaType);
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
