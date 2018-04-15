#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_TILECACHEDRECASTMESHMANAGER_H

#include "cachedrecastmeshmanager.hpp"
#include "tileposition.hpp"

#include <map>
#include <mutex>

namespace DetourNavigator
{
    class TileCachedRecastMeshManager
    {
    public:
        TileCachedRecastMeshManager(const Settings& settings);

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform);

        boost::optional<RecastMeshManager::Object> removeObject(std::size_t id);

        std::shared_ptr<RecastMesh> getMesh(const TilePosition& tilePosition);

    private:
        const Settings& mSettings;
        std::mutex mTilesMutex;
        std::map<TilePosition, CachedRecastMeshManager> mTiles;
        std::unordered_map<std::size_t, std::vector<TilePosition>> mObjectsTilesPositions;
    };
}

#endif
