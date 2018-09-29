#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "cachedrecastmeshmanager.hpp"
#include "tileposition.hpp"

#include <components/misc/guarded.hpp>

#include <map>
#include <mutex>

namespace DetourNavigator
{
    class TileCachedRecastMeshManager
    {
    public:
        TileCachedRecastMeshManager(const Settings& settings);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        boost::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        boost::optional<RecastMeshManager::Water> removeWater(const osg::Vec2i& cellPosition);

        std::shared_ptr<RecastMesh> getMesh(const TilePosition& tilePosition);

        bool hasTile(const TilePosition& tilePosition);

        template <class Function>
        void forEachTilePosition(Function&& function)
        {
            for (const auto& tile : *mTiles.lock())
                function(tile.first);
        }

        std::size_t getRevision() const;

    private:
        const Settings& mSettings;
        Misc::ScopeGuarded<std::map<TilePosition, CachedRecastMeshManager>> mTiles;
        std::unordered_map<ObjectId, std::vector<TilePosition>> mObjectsTilesPositions;
        std::map<osg::Vec2i, std::vector<TilePosition>> mWaterTilesPositions;
        std::size_t mRevision = 0;
    };
}

#endif
