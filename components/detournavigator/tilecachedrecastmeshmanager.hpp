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

        bool hasTile(const TilePosition& tilePosition);

        template <class Function>
        void forEachTilePosition(Function&& function)
        {
            const std::lock_guard<std::mutex> lock(mTilesMutex);
            for (const auto& tile : mTiles)
                function(tile.first);
        }

        std::size_t getRevision() const;

    private:
        const Settings& mSettings;
        std::mutex mTilesMutex;
        std::map<TilePosition, CachedRecastMeshManager> mTiles;
        std::unordered_map<std::size_t, std::vector<TilePosition>> mObjectsTilesPositions;
        std::size_t mRevision = 0;
    };
}

#endif
