#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H

#include "sharednavmesh.hpp"
#include "tileposition.hpp"
#include "navmeshtilescache.hpp"

#include <components/misc/guarded.hpp>

#include <map>

namespace DetourNavigator
{
    class NavMeshCacheItem
    {
    public:
        NavMeshCacheItem(const NavMeshPtr& value, std::size_t generation)
            : mValue(value), mGeneration(generation), mNavMeshRevision(0)
        {
        }

        const dtNavMesh& getValue() const
        {
            return *mValue;
        }

        dtNavMesh& getValue()
        {
            return *mValue;
        }

        std::size_t getGeneration() const
        {
            return mGeneration;
        }

        std::size_t getNavMeshRevision() const
        {
            return mNavMeshRevision;
        }

        void setUsedTile(const TilePosition& tilePosition, NavMeshTilesCache::Value value)
        {
            mUsedTiles[tilePosition] = std::make_pair(std::move(value), NavMeshData());
            ++mNavMeshRevision;
        }

        void setUsedTile(const TilePosition& tilePosition, NavMeshData value)
        {
            mUsedTiles[tilePosition] = std::make_pair(NavMeshTilesCache::Value(), std::move(value));
            ++mNavMeshRevision;
        }

        void removeUsedTile(const TilePosition& tilePosition)
        {
            mUsedTiles.erase(tilePosition);
            ++mNavMeshRevision;
        }

    private:
        NavMeshPtr mValue;
        std::size_t mGeneration;
        std::size_t mNavMeshRevision;
        std::map<TilePosition, std::pair<NavMeshTilesCache::Value, NavMeshData>> mUsedTiles;
    };

    using SharedNavMeshCacheItem = Misc::SharedGuarded<NavMeshCacheItem>;
}

#endif
