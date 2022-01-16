#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "cachedrecastmeshmanager.hpp"
#include "tileposition.hpp"
#include "settingsutils.hpp"
#include "gettilespositions.hpp"
#include "version.hpp"
#include "heightfieldshape.hpp"

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>

namespace DetourNavigator
{
    class TileCachedRecastMeshManager
    {
    public:
        explicit TileCachedRecastMeshManager(const RecastSettings& settings);

        std::string getWorldspace() const;

        void setWorldspace(std::string_view worldspace);

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
            bool changed = false;
            std::vector<TilePosition> newTiles;
            {
                const std::lock_guard lock(mMutex);
                const auto onTilePosition = [&] (const TilePosition& tilePosition)
                {
                    if (std::binary_search(currentTiles.begin(), currentTiles.end(), tilePosition))
                    {
                        newTiles.push_back(tilePosition);
                        if (updateTile(id, transform, areaType, tilePosition, mTiles))
                        {
                            onChangedTile(tilePosition);
                            changed = true;
                        }
                    }
                    else if (addTile(id, shape, transform, areaType, tilePosition, mTiles))
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
                    if (!std::binary_search(newTiles.begin(), newTiles.end(), tile) && removeTile(id, tile, mTiles))
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
            const std::lock_guard lock(mMutex);
            for (auto& [tilePosition, recastMeshManager] : mTiles)
                function(tilePosition, *recastMeshManager);
        }

        std::size_t getRevision() const;

        void reportNavMeshChange(const TilePosition& tilePosition, Version recastMeshVersion, Version navMeshVersion) const;

    private:
        using TilesMap = std::map<TilePosition, std::shared_ptr<CachedRecastMeshManager>>;

        const RecastSettings& mSettings;
        mutable std::mutex mMutex;
        std::string mWorldspace;
        TilesMap mTiles;
        std::unordered_map<ObjectId, std::vector<TilePosition>> mObjectsTilesPositions;
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
