#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "areatype.hpp"
#include "changetype.hpp"
#include "commulativeaabb.hpp"
#include "gettilespositions.hpp"
#include "heightfieldshape.hpp"
#include "objectid.hpp"
#include "recastmesh.hpp"
#include "recastmeshobject.hpp"
#include "tileposition.hpp"
#include "updateguard.hpp"
#include "version.hpp"

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <LinearMath/btTransform.h>

#include <osg/Vec2i>

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace DetourNavigator
{
    class RecastMesh;
    struct TileCachedRecastMeshManagerStats;

    class TileCachedRecastMeshManager
    {
    public:
        explicit TileCachedRecastMeshManager(const RecastSettings& settings);

        ScopedUpdateGuard makeUpdateGuard()
        {
            mMutex.lock();
            return ScopedUpdateGuard(&mUpdateGuard);
        }

        void setRange(const TilesPositionsRange& range, const UpdateGuard* guard);

        TilesPositionsRange getLimitedObjectsRange() const;

        void setWorldspace(ESM::RefId worldspace, const UpdateGuard* guard);

        bool addObject(ObjectId id, const CollisionShape& shape, const btTransform& transform, AreaType areaType,
            const UpdateGuard* guard);

        bool updateObject(ObjectId id, const btTransform& transform, AreaType areaType, const UpdateGuard* guard);

        void removeObject(ObjectId id, const UpdateGuard* guard);

        void addWater(const osg::Vec2i& cellPosition, int cellSize, float level, const UpdateGuard* guard);

        void removeWater(const osg::Vec2i& cellPosition, const UpdateGuard* guard);

        void addHeightfield(
            const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape, const UpdateGuard* guard);

        void removeHeightfield(const osg::Vec2i& cellPosition, const UpdateGuard* guard);

        std::shared_ptr<RecastMesh> getMesh(ESM::RefId worldspace, const TilePosition& tilePosition);

        std::shared_ptr<RecastMesh> getCachedMesh(ESM::RefId worldspace, const TilePosition& tilePosition) const;

        std::shared_ptr<RecastMesh> getNewMesh(ESM::RefId worldspace, const TilePosition& tilePosition) const;

        std::size_t getRevision() const { return mRevision; }

        void reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion);

        void addChangedTile(const TilePosition& tilePosition, ChangeType changeType);

        std::map<osg::Vec2i, ChangeType> takeChangedTiles(const UpdateGuard* guard);

        TileCachedRecastMeshManagerStats getStats() const;

    private:
        struct Report
        {
            std::size_t mRevision;
            Version mNavMeshVersion;
        };

        struct ObjectData
        {
            RecastMeshObject mObject;
            TilesPositionsRange mRange;
            CommulativeAabb mAabb;
            std::size_t mGeneration = 0;
            std::size_t mRevision = 0;
            std::optional<Report> mLastNavMeshReportedChange;
            std::optional<Report> mLastNavMeshReport;
        };

        struct WaterData
        {
            Water mWater;
            std::optional<TilesPositionsRange> mRange;
            std::size_t mRevision;
        };

        struct HeightfieldData
        {
            int mCellSize;
            HeightfieldShape mShape;
            std::optional<TilesPositionsRange> mRange;
            std::size_t mRevision;
        };

        struct CachedTile
        {
            Version mVersion;
            std::shared_ptr<RecastMesh> mRecastMesh;
        };

        using IndexPoint = boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian>;
        using IndexBox = boost::geometry::model::box<IndexPoint>;
        using ObjectIndexValue = std::pair<IndexBox, ObjectData*>;
        using WaterIndexValue = std::pair<IndexBox, std::map<osg::Vec2i, WaterData>::const_iterator>;
        using HeightfieldIndexValue = std::pair<IndexBox, std::map<osg::Vec2i, HeightfieldData>::const_iterator>;

        const RecastSettings& mSettings;
        TilesPositionsRange mRange;
        ESM::RefId mWorldspace;
        std::unordered_map<ObjectId, std::unique_ptr<ObjectData>> mObjects;
        boost::geometry::index::rtree<ObjectIndexValue, boost::geometry::index::quadratic<16>> mObjectIndex;
        std::map<osg::Vec2i, WaterData> mWater;
        std::map<osg::Vec2i, WaterData>::const_iterator mInfiniteWater = mWater.end();
        boost::geometry::index::rtree<WaterIndexValue, boost::geometry::index::linear<4>> mWaterIndex;
        std::map<osg::Vec2i, HeightfieldData> mHeightfields;
        std::map<osg::Vec2i, HeightfieldData>::const_iterator mInfiniteHeightfield = mHeightfields.end();
        boost::geometry::index::rtree<HeightfieldIndexValue, boost::geometry::index::linear<4>> mHeightfieldIndex;
        std::map<osg::Vec2i, ChangeType> mChangedTiles;
        std::map<TilePosition, CachedTile> mCache;
        std::size_t mGeneration = 0;
        std::size_t mRevision = 0;
        mutable std::mutex mMutex;
        UpdateGuard mUpdateGuard{ mMutex };

        inline static IndexPoint makeIndexPoint(const TilePosition& tilePosition);

        inline static IndexBox makeIndexBox(const TilesPositionsRange& range);

        inline static ObjectIndexValue makeObjectIndexValue(const TilesPositionsRange& range, ObjectData* data);

        inline static WaterIndexValue makeWaterIndexValue(
            const TilesPositionsRange& range, std::map<osg::Vec2i, WaterData>::const_iterator it);

        inline static HeightfieldIndexValue makeHeightfieldIndexValue(
            const TilesPositionsRange& range, std::map<osg::Vec2i, HeightfieldData>::const_iterator it);

        inline static auto makeIndexQuery(const TilePosition& tilePosition)
            -> decltype(boost::geometry::index::intersects(IndexBox()));

        inline std::shared_ptr<RecastMesh> makeMesh(const TilePosition& tilePosition) const;

        inline void addChangedTiles(const std::optional<TilesPositionsRange>& range, ChangeType changeType);
    };
}

#endif
