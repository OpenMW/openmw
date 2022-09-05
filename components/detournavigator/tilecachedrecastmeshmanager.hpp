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

#include <components/misc/guarded.hpp>

#include <map>
#include <mutex>
#include <vector>
#include <set>

namespace DetourNavigator
{
    class CachedRecastMeshManager;
    class RecastMesh;

    class TileCachedRecastMeshManager
    {
    public:
        explicit TileCachedRecastMeshManager(const RecastSettings& settings);

        void setBounds(const TileBounds& bounds);

        std::string getWorldspace() const;

        void setWorldspace(std::string_view worldspace);

        bool addObject(ObjectId id, const CollisionShape& shape, const btTransform& transform, AreaType areaType);

        bool updateObject(ObjectId id, const CollisionShape& shape, const btTransform& transform, AreaType areaType);

        void removeObject(ObjectId id);

        void addWater(const osg::Vec2i& cellPosition, int cellSize, float level);

        void removeWater(const osg::Vec2i& cellPosition);

        void addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape);

        void removeHeightfield(const osg::Vec2i& cellPosition);

        std::shared_ptr<RecastMesh> getMesh(std::string_view worldspace, const TilePosition& tilePosition) const;

        std::shared_ptr<RecastMesh> getCachedMesh(std::string_view worldspace, const TilePosition& tilePosition) const;

        std::shared_ptr<RecastMesh> getNewMesh(std::string_view worldspace, const TilePosition& tilePosition) const;

        template <class Function>
        void forEachTile(Function&& function) const
        {
            const auto& locked = mWorldspaceTiles.lockConst();
            for (const auto& [tilePosition, recastMeshManager] : locked->mTiles)
                function(tilePosition, *recastMeshManager);
        }

        std::size_t getRevision() const { return mRevision; }

        void reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion) const;

        void addChangedTile(const TilePosition& tilePosition, ChangeType changeType);

        std::map<osg::Vec2i, ChangeType> takeChangedTiles() { return std::move(mChangedTiles); }

    private:
        using TilesMap = std::map<TilePosition, std::shared_ptr<CachedRecastMeshManager>>;

        struct ObjectData
        {
            const CollisionShape mShape;
            const btTransform mTransform;
            const AreaType mAreaType;
            std::set<TilePosition> mTiles;
        };

        struct WorldspaceTiles
        {
            std::string mWorldspace;
            TilesMap mTiles;
        };

        const RecastSettings& mSettings;
        TileBounds mBounds;
        TilesPositionsRange mRange;
        Misc::ScopeGuarded<WorldspaceTiles> mWorldspaceTiles;
        std::unordered_map<ObjectId, ObjectData> mObjects;
        std::map<osg::Vec2i, std::vector<TilePosition>> mWaterTilesPositions;
        std::map<osg::Vec2i, std::vector<TilePosition>> mHeightfieldTilesPositions;
        std::map<osg::Vec2i, ChangeType> mChangedTiles;
        std::size_t mRevision = 0;
        std::size_t mTilesGeneration = 0;

        inline bool addTile(ObjectId id, const CollisionShape& shape, const btTransform& transform,
                AreaType areaType, const TilePosition& tilePosition, TilesMap& tiles);

        inline bool removeTile(ObjectId id, const TilePosition& tilePosition, TilesMap& tiles);

        inline std::shared_ptr<CachedRecastMeshManager> getManager(std::string_view worldspace,
                const TilePosition& tilePosition) const;
    };
}

#endif
