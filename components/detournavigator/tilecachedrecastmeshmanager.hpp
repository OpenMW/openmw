#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "tileposition.hpp"
#include "gettilespositions.hpp"
#include "version.hpp"
#include "heightfieldshape.hpp"
#include "changetype.hpp"
#include "objectid.hpp"
#include "areatype.hpp"
#include "recastmeshobject.hpp"
#include "commulativeaabb.hpp"
#include "version.hpp"
#include "recastmesh.hpp"

#include <components/misc/guarded.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <map>
#include <mutex>
#include <vector>
#include <optional>

namespace DetourNavigator
{
    class RecastMesh;

    class TileCachedRecastMeshManager
    {
    public:
        explicit TileCachedRecastMeshManager(const RecastSettings& settings);

        void setBounds(const TileBounds& bounds);

        TilesPositionsRange getRange() const;

        void setWorldspace(std::string_view worldspace);

        bool addObject(ObjectId id, const CollisionShape& shape, const btTransform& transform, AreaType areaType);

        bool updateObject(ObjectId id, const btTransform& transform, AreaType areaType);

        void removeObject(ObjectId id);

        void addWater(const osg::Vec2i& cellPosition, int cellSize, float level);

        void removeWater(const osg::Vec2i& cellPosition);

        void addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape);

        void removeHeightfield(const osg::Vec2i& cellPosition);

        std::shared_ptr<RecastMesh> getMesh(std::string_view worldspace, const TilePosition& tilePosition);

        std::shared_ptr<RecastMesh> getCachedMesh(std::string_view worldspace, const TilePosition& tilePosition) const;

        std::shared_ptr<RecastMesh> getNewMesh(std::string_view worldspace, const TilePosition& tilePosition) const;

        std::size_t getRevision() const { return mRevision; }

        void reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion);

        void addChangedTile(const TilePosition& tilePosition, ChangeType changeType);

        std::map<osg::Vec2i, ChangeType> takeChangedTiles();

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
        TileBounds mBounds;
        TilesPositionsRange mRange;
        std::string mWorldspace;
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

        inline static IndexPoint makeIndexPoint(const TilePosition& tilePosition);

        inline static IndexBox makeIndexBox(const TilesPositionsRange& range);

        inline static ObjectIndexValue makeObjectIndexValue(const TilesPositionsRange& range, ObjectData* data);

        inline static WaterIndexValue makeWaterIndexValue(const TilesPositionsRange& range,
            std::map<osg::Vec2i, WaterData>::const_iterator it);

        inline static HeightfieldIndexValue makeHeightfieldIndexValue(const TilesPositionsRange& range,
            std::map<osg::Vec2i, HeightfieldData>::const_iterator it);

        inline static auto makeIndexQuery(const TilePosition& tilePosition)
            -> decltype(boost::geometry::index::intersects(IndexBox()));

        inline std::shared_ptr<RecastMesh> makeMesh(const TilePosition& tilePosition) const;

        inline void addChangedTiles(const std::optional<TilesPositionsRange>& range, ChangeType changeType);
    };
}

#endif
