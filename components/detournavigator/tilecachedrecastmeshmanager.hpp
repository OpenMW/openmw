#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "cachedrecastmeshmanager.hpp"
#include "tileposition.hpp"
#include "settingsutils.hpp"
#include "gettilespositions.hpp"
#include "version.hpp"
#include "heightfieldshape.hpp"
#include "changetype.hpp"

#include <components/misc/guarded.hpp>

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>
#include <set>

namespace DetourNavigator
{
    class TileCachedRecastMeshManager
    {
    public:
        explicit TileCachedRecastMeshManager(const RecastSettings& settings);

        TileBounds getBounds() const;

        std::vector<std::pair<TilePosition, ChangeType>> setBounds(const TileBounds& bounds);

        std::string getWorldspace() const;

        void setWorldspace(std::string_view worldspace);

        template <class OnChangedTile>
        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
            const AreaType areaType, OnChangedTile&& onChangedTile)
        {
            auto it = mObjects.find(id);
            if (it != mObjects.end())
                return false;
            const TilesPositionsRange objectRange = makeTilesPositionsRange(shape.getShape(), transform, mSettings);
            const TilesPositionsRange range = getIntersection(mRange, objectRange);
            std::set<TilePosition> tilesPositions;
            if (range.mBegin != range.mEnd)
            {
                const auto locked = mWorldspaceTiles.lock();
                getTilesPositions(range,
                    [&] (const TilePosition& tilePosition)
                    {
                        if (addTile(id, shape, transform, areaType, tilePosition, locked->mTiles))
                            tilesPositions.insert(tilePosition);
                    });
            }
            it = mObjects.emplace_hint(it, id, ObjectData {shape, transform, areaType, std::move(tilesPositions)});
            std::for_each(it->second.mTiles.begin(), it->second.mTiles.end(), std::forward<OnChangedTile>(onChangedTile));
            ++mRevision;
            return true;
        }

        template <class OnChangedTile>
        bool updateObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
            const AreaType areaType, OnChangedTile&& onChangedTile)
        {
            const auto object = mObjects.find(id);
            if (object == mObjects.end())
                return false;
            auto& data = object->second;
            bool changed = false;
            std::set<TilePosition> newTiles;
            {
                const TilesPositionsRange objectRange = makeTilesPositionsRange(shape.getShape(), transform, mSettings);
                const TilesPositionsRange range = getIntersection(mRange, objectRange);
                const auto locked = mWorldspaceTiles.lock();
                const auto onTilePosition = [&] (const TilePosition& tilePosition)
                {
                    if (data.mTiles.find(tilePosition) != data.mTiles.end())
                    {
                        newTiles.insert(tilePosition);
                        if (updateTile(id, transform, areaType, tilePosition, locked->mTiles))
                        {
                            onChangedTile(tilePosition, ChangeType::update);
                            changed = true;
                        }
                    }
                    else if (addTile(id, shape, transform, areaType, tilePosition, locked->mTiles))
                    {
                        newTiles.insert(tilePosition);
                        onChangedTile(tilePosition, ChangeType::add);
                        changed = true;
                    }
                };
                getTilesPositions(range, onTilePosition);
                for (const auto& tile : data.mTiles)
                {
                    if (newTiles.find(tile) == newTiles.end() && removeTile(id, tile, locked->mTiles))
                    {
                        onChangedTile(tile, ChangeType::remove);
                        changed = true;
                    }
                }
            }
            if (changed)
            {
                data.mTiles = std::move(newTiles);
                ++mRevision;
            }
            return changed;
        }

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        bool addWater(const osg::Vec2i& cellPosition, int cellSize, float level);

        std::optional<Water> removeWater(const osg::Vec2i& cellPosition);

        bool addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape);

        std::optional<SizedHeightfieldShape> removeHeightfield(const osg::Vec2i& cellPosition);

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

        std::size_t getRevision() const;

        void reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion) const;

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
        std::size_t mRevision = 0;
        std::size_t mTilesGeneration = 0;

        bool addTile(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                const AreaType areaType, const TilePosition& tilePosition, TilesMap& tiles);

        bool updateTile(const ObjectId id, const btTransform& transform, const AreaType areaType,
                const TilePosition& tilePosition, TilesMap& tiles);

        std::optional<RemovedRecastMeshObject> removeTile(const ObjectId id, const TilePosition& tilePosition,
                TilesMap& tiles);

        inline std::shared_ptr<CachedRecastMeshManager> getManager(std::string_view worldspace,
                const TilePosition& tilePosition) const;
    };
}

#endif
