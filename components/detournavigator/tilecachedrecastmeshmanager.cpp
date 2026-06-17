#include "tilecachedrecastmeshmanager.hpp"

#include "changetype.hpp"
#include "gettilespositions.hpp"
#include "recastmeshbuilder.hpp"
#include "settingsutils.hpp"
#include "stats.hpp"
#include "updateguard.hpp"

#include <components/bullethelpers/aabb.hpp>
#include <components/misc/convert.hpp>

#include <boost/geometry/geometry.hpp>

#include <limits>

namespace DetourNavigator
{
    namespace
    {
        const TilesPositionsRange infiniteRange{
            .mBegin = TilePosition(std::numeric_limits<int>::min(), std::numeric_limits<int>::min()),
            .mEnd = TilePosition(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()),
        };

        struct AddHeightfield
        {
            osg::Vec2i mCellPosition;
            int mCellSize;
            RecastMeshBuilder& mBuilder;

            void operator()(const HeightfieldSurface& v)
            {
                mBuilder.addHeightfield(mCellPosition, mCellSize, v.mHeights, v.mSize, v.mMinHeight, v.mMaxHeight);
            }

            void operator()(HeightfieldPlane v) { mBuilder.addHeightfield(mCellPosition, mCellSize, v.mHeight); }
        };

        TilePosition makeTilePosition(const boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian>& v)
        {
            return TilePosition(v.get<0>(), v.get<1>());
        }

        template <class Mutex>
        class MaybeLockGuard
        {
        public:
            explicit MaybeLockGuard(Mutex& mutex, const UpdateGuard* guard)
                : mImpl(guard == nullptr ? std::optional<std::unique_lock<Mutex>>(mutex) : std::nullopt)
            {
            }

        private:
            const std::optional<std::unique_lock<Mutex>> mImpl;
        };

        TilesPositionsRange getIndexRange(const auto& index)
        {
            const auto bounds = index.bounds();
            return TilesPositionsRange{
                .mBegin = makeTilePosition(bounds.min_corner()),
                .mEnd = makeTilePosition(bounds.max_corner()) + TilePosition(1, 1),
            };
        }
    }

    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const RecastSettings& settings)
        : mSettings(settings)
        , mRange(infiniteRange)
    {
    }

    void TileCachedRecastMeshManager::setRange(const TilesPositionsRange& range, const UpdateGuard* guard)
    {
        if (mRange == range)
            return;

        bool changed = false;
        if (mRange != infiniteRange)
        {
            for (const auto& [id, data] : mObjects)
            {
                const TilesPositionsRange objectRange
                    = makeTilesPositionsRange(data->mObject.getShape(), data->mObject.getTransform(), mSettings);

                getTilesPositions(getIntersection(mRange, objectRange), [&](const TilePosition& v) {
                    if (!isInTilesPositionsRange(range, v))
                    {
                        addChangedTile(v, ChangeType::remove);
                        changed = true;
                    }
                });

                getTilesPositions(getIntersection(range, objectRange), [&](const TilePosition& v) {
                    if (!isInTilesPositionsRange(mRange, v))
                    {
                        addChangedTile(v, ChangeType::add);
                        changed = true;
                    }
                });
            }

            getTilesPositions(mRange, [&](const TilePosition& v) {
                if (!isInTilesPositionsRange(range, v))
                    mCache.erase(v);
            });
        }

        const MaybeLockGuard lock(mMutex, guard);

        if (changed)
            ++mRevision;

        mRange = range;
    }

    TilesPositionsRange TileCachedRecastMeshManager::getLimitedObjectsRange() const
    {
        std::optional<TilesPositionsRange> result;
        if (!mWater.empty())
            result = getIndexRange(mWaterIndex);
        if (!mHeightfields.empty())
        {
            const TilesPositionsRange range = getIndexRange(mHeightfieldIndex);
            if (result.has_value())
                result = getUnion(*result, range);
            else
                result = range;
        }
        if (!mObjects.empty())
        {
            const TilesPositionsRange range = getIndexRange(mObjectIndex);
            if (result.has_value())
                result = getUnion(*result, range);
            else
                result = range;
        }
        if (result.has_value())
            return getIntersection(mRange, *result);
        return {};
    }

    void TileCachedRecastMeshManager::setWorldspace(ESM::RefId worldspace, const UpdateGuard* guard)
    {
        const MaybeLockGuard lock(mMutex, guard);
        if (mWorldspace == worldspace)
            return;
        mWorldspace = worldspace;
        ++mGeneration;
        ++mRevision;
        mObjectIndex.clear();
        mObjects.clear();
        mWater.clear();
        mHeightfields.clear();
        mCache.clear();
    }

    bool TileCachedRecastMeshManager::addObject(ObjectId id, const CollisionShape& shape, const btTransform& transform,
        const AreaType areaType, const UpdateGuard* guard)
    {
        const TilesPositionsRange range = makeTilesPositionsRange(shape.getShape(), transform, mSettings);
        {
            const MaybeLockGuard lock(mMutex, guard);
            const auto it = mObjects.find(id);
            if (it != mObjects.end())
                return false;
            const std::size_t revision = mRevision + 1;
            ObjectData* const dataPtr
                = mObjects
                      .emplace_hint(it, id,
                          std::unique_ptr<ObjectData>(new ObjectData{
                              .mObject = RecastMeshObject(shape, transform, areaType),
                              .mRange = range,
                              .mAabb = CommulativeAabb(revision, BulletHelpers::getAabb(shape.getShape(), transform)),
                              .mGeneration = mGeneration,
                              .mRevision = revision,
                              .mLastNavMeshReportedChange = {},
                              .mLastNavMeshReport = {},
                          }))
                      ->second.get();
            assert(range.mBegin != range.mEnd);
            mObjectIndex.insert(makeObjectIndexValue(range, dataPtr));
            mRevision = revision;
        }
        getTilesPositions(
            getIntersection(range, mRange), [&](const TilePosition& v) { addChangedTile(v, ChangeType::add); });
        return true;
    }

    bool TileCachedRecastMeshManager::updateObject(
        ObjectId id, const btTransform& transform, const AreaType areaType, const UpdateGuard* guard)
    {
        TilesPositionsRange newRange;
        TilesPositionsRange oldRange;
        {
            const MaybeLockGuard lock(mMutex, guard);
            const auto it = mObjects.find(id);
            if (it == mObjects.end())
                return false;
            if (!it->second->mObject.update(transform, areaType))
                return false;
            const std::size_t lastChangeRevision = it->second->mLastNavMeshReportedChange.has_value()
                ? it->second->mLastNavMeshReportedChange->mRevision
                : mRevision;
            const btCollisionShape& shape = it->second->mObject.getShape();
            if (!it->second->mAabb.update(lastChangeRevision, BulletHelpers::getAabb(shape, transform)))
                return false;
            newRange = makeTilesPositionsRange(shape, transform, mSettings);
            oldRange = it->second->mRange;
            if (newRange != oldRange)
            {
                mObjectIndex.remove(makeObjectIndexValue(oldRange, it->second.get()));
                mObjectIndex.insert(makeObjectIndexValue(newRange, it->second.get()));
                it->second->mRange = newRange;
            }
            ++mRevision;
            it->second->mRevision = mRevision;
        }
        if (newRange == oldRange)
        {
            getTilesPositions(getIntersection(newRange, mRange),
                [&](const TilePosition& v) { addChangedTile(v, ChangeType::update); });
        }
        else
        {
            getTilesPositions(getIntersection(newRange, mRange), [&](const TilePosition& v) {
                const ChangeType changeType
                    = isInTilesPositionsRange(oldRange, v) ? ChangeType::update : ChangeType::add;
                addChangedTile(v, changeType);
            });
            getTilesPositions(getIntersection(oldRange, mRange), [&](const TilePosition& v) {
                if (!isInTilesPositionsRange(newRange, v))
                    addChangedTile(v, ChangeType::remove);
            });
        }
        return true;
    }

    void TileCachedRecastMeshManager::removeObject(ObjectId id, const UpdateGuard* guard)
    {
        TilesPositionsRange range;
        {
            const MaybeLockGuard lock(mMutex, guard);
            const auto it = mObjects.find(id);
            if (it == mObjects.end())
                return;
            range = it->second->mRange;
            mObjectIndex.remove(makeObjectIndexValue(range, it->second.get()));
            mObjects.erase(it);
            ++mRevision;
        }
        getTilesPositions(
            getIntersection(range, mRange), [&](const TilePosition& v) { addChangedTile(v, ChangeType::remove); });
    }

    void TileCachedRecastMeshManager::addWater(
        const osg::Vec2i& cellPosition, const int cellSize, const float level, const UpdateGuard* guard)
    {
        const btVector3 shift = Misc::Convert::toBullet(getWaterShift3d(cellPosition, cellSize, level));
        const std::optional<TilesPositionsRange> range = cellSize == std::numeric_limits<int>::max()
            ? std::optional<TilesPositionsRange>()
            : makeTilesPositionsRange(cellSize, shift, mSettings);
        {
            const MaybeLockGuard lock(mMutex, guard);
            auto it = mWater.find(cellPosition);
            if (it != mWater.end())
                return;
            const std::size_t revision = mRevision + 1;
            it = mWater.emplace_hint(it, cellPosition,
                WaterData{
                    .mWater = Water{ .mCellSize = cellSize, .mLevel = level },
                    .mRange = range,
                    .mRevision = revision,
                });
            if (range.has_value())
                mWaterIndex.insert(makeWaterIndexValue(*range, it));
            else
                mInfiniteWater = it;
            mRevision = revision;
        }
        addChangedTiles(range, ChangeType::add);
    }

    void TileCachedRecastMeshManager::removeWater(const osg::Vec2i& cellPosition, const UpdateGuard* guard)
    {
        std::optional<TilesPositionsRange> range;
        {
            const MaybeLockGuard lock(mMutex, guard);
            const auto it = mWater.find(cellPosition);
            if (it == mWater.end())
                return;
            range = it->second.mRange;
            if (range.has_value())
                mWaterIndex.remove(makeWaterIndexValue(*range, it));
            else
                mInfiniteWater = mWater.end();
            mWater.erase(it);
            ++mRevision;
        }
        addChangedTiles(range, ChangeType::remove);
    }

    void TileCachedRecastMeshManager::addHeightfield(
        const osg::Vec2i& cellPosition, const int cellSize, const HeightfieldShape& shape, const UpdateGuard* guard)
    {
        const btVector3 shift = getHeightfieldShift(shape, cellPosition, cellSize);
        const std::optional<TilesPositionsRange> range = cellSize == std::numeric_limits<int>::max()
            ? std::optional<TilesPositionsRange>()
            : makeTilesPositionsRange(cellSize, shift, mSettings);
        {
            const MaybeLockGuard lock(mMutex, guard);
            auto it = mHeightfields.find(cellPosition);
            if (it != mHeightfields.end())
                return;
            const std::size_t revision = mRevision + 1;
            it = mHeightfields.emplace_hint(it, cellPosition,
                HeightfieldData{
                    .mCellSize = cellSize,
                    .mShape = shape,
                    .mRange = range,
                    .mRevision = revision,
                });
            if (range.has_value())
                mHeightfieldIndex.insert(makeHeightfieldIndexValue(*range, it));
            else
                mInfiniteHeightfield = it;
            mRevision = revision;
        }
        addChangedTiles(range, ChangeType::add);
    }

    void TileCachedRecastMeshManager::removeHeightfield(const osg::Vec2i& cellPosition, const UpdateGuard* guard)
    {
        std::optional<TilesPositionsRange> range;
        {
            const MaybeLockGuard lock(mMutex, guard);
            const auto it = mHeightfields.find(cellPosition);
            if (it == mHeightfields.end())
                return;
            range = it->second.mRange;
            if (range.has_value())
                mHeightfieldIndex.remove(makeHeightfieldIndexValue(*range, it));
            else
                mInfiniteHeightfield = mHeightfields.end();
            mHeightfields.erase(it);
            ++mRevision;
        }
        addChangedTiles(range, ChangeType::remove);
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getMesh(
        ESM::RefId worldspace, const TilePosition& tilePosition)
    {
        {
            const std::lock_guard lock(mMutex);
            if (mWorldspace != worldspace)
                return nullptr;
            if (!isInTilesPositionsRange(mRange, tilePosition))
                return nullptr;
            const auto it = mCache.find(tilePosition);
            if (it != mCache.end() && it->second.mRecastMesh->getVersion() == it->second.mVersion)
                return it->second.mRecastMesh;
        }
        auto result = makeMesh(tilePosition);
        if (result != nullptr)
        {
            const std::lock_guard lock(mMutex);
            mCache.insert_or_assign(tilePosition,
                CachedTile{
                    .mVersion = result->getVersion(),
                    .mRecastMesh = result,
                });
        }
        return result;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getCachedMesh(
        ESM::RefId worldspace, const TilePosition& tilePosition) const
    {
        const std::lock_guard lock(mMutex);
        if (mWorldspace != worldspace)
            return nullptr;
        if (!isInTilesPositionsRange(mRange, tilePosition))
            return nullptr;
        const auto it = mCache.find(tilePosition);
        if (it == mCache.end())
            return nullptr;
        return it->second.mRecastMesh;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getNewMesh(
        ESM::RefId worldspace, const TilePosition& tilePosition) const
    {
        {
            const std::lock_guard lock(mMutex);
            if (mWorldspace != worldspace)
                return nullptr;
        }
        return makeMesh(tilePosition);
    }

    void TileCachedRecastMeshManager::reportNavMeshChange(
        const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion)
    {
        const std::lock_guard lock(mMutex);
        for (auto it = mObjectIndex.qbegin(makeIndexQuery(tilePosition)); it != mObjectIndex.qend(); ++it)
        {
            ObjectData& object = *it->second;
            if (recastMeshVersion.mGeneration != object.mGeneration)
                continue;
            if (object.mLastNavMeshReport.has_value() && navMeshVersion < object.mLastNavMeshReport->mNavMeshVersion)
                continue;
            object.mLastNavMeshReport = { recastMeshVersion.mRevision, navMeshVersion };
            if (!object.mLastNavMeshReportedChange.has_value()
                || object.mLastNavMeshReportedChange->mNavMeshVersion < object.mLastNavMeshReport->mNavMeshVersion)
                object.mLastNavMeshReportedChange = object.mLastNavMeshReport;
        }
    }

    void TileCachedRecastMeshManager::addChangedTile(const TilePosition& tilePosition, const ChangeType changeType)
    {
        auto tile = mChangedTiles.find(tilePosition);
        if (tile == mChangedTiles.end())
            mChangedTiles.emplace(tilePosition, changeType);
        else
            tile->second = changeType == ChangeType::remove ? changeType : tile->second;
    }

    std::map<osg::Vec2i, ChangeType> TileCachedRecastMeshManager::takeChangedTiles(const UpdateGuard* guard)
    {
        {
            const MaybeLockGuard lock(mMutex, guard);
            for (const auto& [tilePosition, changeType] : mChangedTiles)
                if (const auto it = mCache.find(tilePosition); it != mCache.end())
                    ++it->second.mVersion.mRevision;
        }
        return std::move(mChangedTiles);
    }

    TileCachedRecastMeshManagerStats TileCachedRecastMeshManager::getStats() const
    {
        const std::lock_guard lock(mMutex);
        return TileCachedRecastMeshManagerStats{
            .mTiles = mCache.size(),
            .mObjects = mObjects.size(),
            .mHeightfields = mHeightfields.size(),
            .mWater = mWater.size(),
        };
    }

    TileCachedRecastMeshManager::IndexPoint TileCachedRecastMeshManager::makeIndexPoint(
        const TilePosition& tilePosition)
    {
        return IndexPoint(tilePosition.x(), tilePosition.y());
    }

    TileCachedRecastMeshManager::IndexBox TileCachedRecastMeshManager::makeIndexBox(const TilesPositionsRange& range)
    {
        assert(range.mBegin != range.mEnd);
        return IndexBox(makeIndexPoint(range.mBegin), makeIndexPoint(range.mEnd - TilePosition(1, 1)));
    }

    TileCachedRecastMeshManager::ObjectIndexValue TileCachedRecastMeshManager::makeObjectIndexValue(
        const TilesPositionsRange& range, ObjectData* id)
    {
        return { makeIndexBox(range), id };
    }

    TileCachedRecastMeshManager::WaterIndexValue TileCachedRecastMeshManager::makeWaterIndexValue(
        const TilesPositionsRange& range, std::map<osg::Vec2i, WaterData>::const_iterator it)
    {
        return { makeIndexBox(range), it };
    }

    TileCachedRecastMeshManager::HeightfieldIndexValue TileCachedRecastMeshManager::makeHeightfieldIndexValue(
        const TilesPositionsRange& range, std::map<osg::Vec2i, HeightfieldData>::const_iterator it)
    {
        return { makeIndexBox(range), it };
    }

    auto TileCachedRecastMeshManager::makeIndexQuery(const TilePosition& tilePosition)
        -> decltype(boost::geometry::index::intersects(IndexBox()))
    {
        const IndexPoint point = makeIndexPoint(tilePosition);
        return boost::geometry::index::intersects(IndexBox(point, point));
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::makeMesh(const TilePosition& tilePosition) const
    {
        RecastMeshBuilder builder(makeRealTileBoundsWithBorder(mSettings, tilePosition));
        using Object = std::tuple<osg::ref_ptr<const Resource::BulletShapeInstance>, ObjectTransform,
            std::reference_wrapper<const btCollisionShape>, btTransform, AreaType>;
        std::vector<Object> objects;
        Version version;
        bool hasInput = false;
        {
            const std::lock_guard lock(mMutex);
            for (auto it = mWaterIndex.qbegin(makeIndexQuery(tilePosition)); it != mWaterIndex.qend(); ++it)
            {
                const auto& [cellPosition, data] = *it->second;
                builder.addWater(cellPosition, data.mWater);
                hasInput = true;
            }
            for (auto it = mHeightfieldIndex.qbegin(makeIndexQuery(tilePosition)); it != mHeightfieldIndex.qend(); ++it)
            {
                const auto& [cellPosition, data] = *it->second;
                std::visit(AddHeightfield{ cellPosition, data.mCellSize, builder }, data.mShape);
                hasInput = true;
            }
            objects.reserve(mObjects.size());
            for (auto it = mObjectIndex.qbegin(makeIndexQuery(tilePosition)); it != mObjectIndex.qend(); ++it)
            {
                const auto& object = it->second->mObject;
                objects.emplace_back(object.getInstance(), object.getObjectTransform(), object.getShape(),
                    object.getTransform(), object.getAreaType());
                hasInput = true;
            }
            if (hasInput)
            {
                if (mInfiniteWater != mWater.end())
                    builder.addWater(mInfiniteWater->first, mInfiniteWater->second.mWater);
                if (mInfiniteHeightfield != mHeightfields.end())
                    std::visit(
                        AddHeightfield{ mInfiniteHeightfield->first, mInfiniteHeightfield->second.mCellSize, builder },
                        mInfiniteHeightfield->second.mShape);
                version.mGeneration = mGeneration;
                version.mRevision = mRevision;
            }
        }
        if (!hasInput)
            return nullptr;
        for (const auto& [instance, objectTransform, shape, transform, areaType] : objects)
            builder.addObject(shape, transform, areaType, instance->getSource(), objectTransform);
        return std::move(builder).create(version);
    }

    void TileCachedRecastMeshManager::addChangedTiles(
        const std::optional<TilesPositionsRange>& range, ChangeType changeType)
    {
        if (range.has_value())
            getTilesPositions(*range, [&](const TilePosition& v) { addChangedTile(v, changeType); });
    }
}
