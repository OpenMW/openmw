#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "cachedrecastmeshmanager.hpp"
#include "tileposition.hpp"
#include "settingsutils.hpp"
#include "gettilespositions.hpp"
#include "version.hpp"

#include <components/misc/guarded.hpp>

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>

namespace DetourNavigator
{
    class TileCachedRecastMeshManager
    {
    public:
        TileCachedRecastMeshManager(const Settings& settings);

        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        template <class OnChangedTile>
        bool updateObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
            const AreaType areaType, OnChangedTile&& onChangedTile)
        {
            const auto object = mObjectsTilesPositions.find(id);
            if (object == mObjectsTilesPositions.end())
                return false;
            auto& currentTiles = object->second;
            const auto border = getBorderSize(mSettings);
            bool changed = false;
            std::vector<TilePosition> newTiles;
            {
                auto tiles = mTiles.lock();
                const auto onTilePosition = [&] (const TilePosition& tilePosition)
                {
                    if (std::binary_search(currentTiles.begin(), currentTiles.end(), tilePosition))
                    {
                        newTiles.push_back(tilePosition);
                        if (updateTile(id, transform, areaType, tilePosition, tiles.get()))
                        {
                            onChangedTile(tilePosition);
                            changed = true;
                        }
                    }
                    else if (addTile(id, shape, transform, areaType, tilePosition, border, tiles.get()))
                    {
                        newTiles.push_back(tilePosition);
                        onChangedTile(tilePosition);
                        changed = true;
                    }
                };
                getTilesPositions(shape.getShape(), transform, mSettings, onTilePosition);
                std::sort(newTiles.begin(), newTiles.end());
                for (const auto& tile : currentTiles)
                {
                    if (!std::binary_search(newTiles.begin(), newTiles.end(), tile) && removeTile(id, tile, tiles.get()))
                    {
                        onChangedTile(tile);
                        changed = true;
                    }
                }
            }
            if (changed)
            {
                currentTiles = std::move(newTiles);
                ++mRevision;
            }
            return changed;
        }

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        std::optional<RecastMeshManager::Water> removeWater(const osg::Vec2i& cellPosition);

        std::shared_ptr<RecastMesh> getMesh(const TilePosition& tilePosition);

        bool hasTile(const TilePosition& tilePosition);

        template <class Function>
        void forEachTile(Function&& function)
        {
            for (auto& [tilePosition, recastMeshManager] : *mTiles.lock())
                function(tilePosition, *recastMeshManager);
        }

        std::size_t getRevision() const;

        void reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion);

    private:
        using TilesMap = std::map<TilePosition, std::shared_ptr<CachedRecastMeshManager>>;

        const Settings& mSettings;
        Misc::ScopeGuarded<TilesMap> mTiles;
        std::unordered_map<ObjectId, std::vector<TilePosition>> mObjectsTilesPositions;
        std::map<osg::Vec2i, std::vector<TilePosition>> mWaterTilesPositions;
        std::size_t mRevision = 0;
        std::size_t mTilesGeneration = 0;

        bool addTile(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                const AreaType areaType, const TilePosition& tilePosition, float border, TilesMap& tiles);

        bool updateTile(const ObjectId id, const btTransform& transform, const AreaType areaType,
                const TilePosition& tilePosition, TilesMap& tiles);

        std::optional<RemovedRecastMeshObject> removeTile(const ObjectId id, const TilePosition& tilePosition,
                TilesMap& tiles);
    };
}

#endif
