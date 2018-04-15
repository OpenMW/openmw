#include "tilecachedrecastmeshmanager.hpp"
#include "makenavmesh.hpp"
#include "gettilespositions.hpp"
#include "settingsutils.hpp"

namespace DetourNavigator
{
    TileCachedRecastMeshManager::TileCachedRecastMeshManager(const Settings& settings)
        : mSettings(settings)
    {
    }

    bool TileCachedRecastMeshManager::addObject(std::size_t id, const btCollisionShape& shape,
                                                const btTransform& transform)
    {
        bool result = false;
        auto& tilesPositions = mObjectsTilesPositions[id];
        const auto border = getBorderSize(mSettings);
        getTilesPositions(shape, transform, mSettings, [&] (const TilePosition& tilePosition)
            {
                std::unique_lock<std::mutex> lock(mTilesMutex);
                auto tile = mTiles.find(tilePosition);
                if (tile == mTiles.end())
                {
                    auto tileBounds = makeTileBounds(mSettings, tilePosition);
                    tileBounds.mMin -= osg::Vec2f(border, border);
                    tileBounds.mMax += osg::Vec2f(border, border);
                    tile = mTiles.insert(std::make_pair(tilePosition,
                            CachedRecastMeshManager(mSettings, tileBounds))).first;
                }
                if (tile->second.addObject(id, shape, transform))
                {
                    lock.unlock();
                    tilesPositions.push_back(tilePosition);
                    result = true;
                }
            });
        return result;
    }

    boost::optional<RecastMeshManager::Object> TileCachedRecastMeshManager::removeObject(std::size_t id)
    {
        const auto object = mObjectsTilesPositions.find(id);
        if (object == mObjectsTilesPositions.end())
            return boost::none;
        boost::optional<RecastMeshManager::Object> result;
        for (const auto& tilePosition : object->second)
        {
            std::unique_lock<std::mutex> lock(mTilesMutex);
            const auto tile = mTiles.find(tilePosition);
            if (tile == mTiles.end())
                continue;
            const auto tileResult = tile->second.removeObject(id);
            if (tile->second.isEmpty())
                mTiles.erase(tile);
            lock.unlock();
            if (tileResult && !result)
                result = tileResult;
        }
        return result;
    }

    std::shared_ptr<RecastMesh> TileCachedRecastMeshManager::getMesh(const TilePosition& tilePosition)
    {
        const std::lock_guard<std::mutex> lock(mTilesMutex);
        const auto it = mTiles.find(tilePosition);
        if (it == mTiles.end())
            return nullptr;
        return it->second.getMesh();
    }
}
