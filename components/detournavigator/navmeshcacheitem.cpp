#include "tileposition.hpp"
#include "navmeshtilescache.hpp"
#include "dtstatus.hpp"
#include "navmeshtileview.hpp"
#include "navmeshcacheitem.hpp"
#include "navmeshdata.hpp"

#include <components/misc/guarded.hpp>

#include <DetourNavMesh.h>

namespace
{
    using DetourNavigator::TilePosition;

    const dtMeshTile* getTile(const dtNavMesh& navMesh, const TilePosition& position)
    {
        const int layer = 0;
        return navMesh.getTileAt(position.x(), position.y(), layer);
    }

    bool removeTile(dtNavMesh& navMesh, const TilePosition& position)
    {
        const int layer = 0;
        const auto tileRef = navMesh.getTileRefAt(position.x(), position.y(), layer);
        if (tileRef == 0)
            return false;
        unsigned char** const data = nullptr;
        int* const dataSize = nullptr;
        return dtStatusSucceed(navMesh.removeTile(tileRef, data, dataSize));
    }

    dtStatus addTile(dtNavMesh& navMesh, unsigned char* data, int size)
    {
        const int doNotTransferOwnership = 0;
        const dtTileRef lastRef = 0;
        dtTileRef* const result = nullptr;
        return navMesh.addTile(data, size, doNotTransferOwnership, lastRef, result);
    }
}

namespace DetourNavigator
{
    UpdateNavMeshStatus NavMeshCacheItem::updateTile(const TilePosition& position, NavMeshTilesCache::Value&& cached,
        NavMeshData&& navMeshData)
    {
        const dtMeshTile* currentTile = ::getTile(*mImpl, position);
        if (currentTile != nullptr
            && asNavMeshTileConstView(*currentTile) == asNavMeshTileConstView(navMeshData.mValue.get()))
        {
            return UpdateNavMeshStatus::ignored;
        }
        const auto removed = ::removeTile(*mImpl, position);
        const auto addStatus = addTile(*mImpl, navMeshData.mValue.get(), navMeshData.mSize);
        if (dtStatusSucceed(addStatus))
        {
            mUsedTiles[position] = std::make_pair(std::move(cached), std::move(navMeshData));
            ++mNavMeshRevision;
            return UpdateNavMeshStatusBuilder().added(true).removed(removed).getResult();
        }
        else
        {
            if (removed)
            {
                mUsedTiles.erase(position);
                ++mNavMeshRevision;
            }
            return UpdateNavMeshStatusBuilder().removed(removed).failed((addStatus & DT_OUT_OF_MEMORY) != 0).getResult();
        }
    }

    UpdateNavMeshStatus NavMeshCacheItem::removeTile(const TilePosition& position)
    {
        const auto removed = ::removeTile(*mImpl, position);
        if (removed)
        {
            mUsedTiles.erase(position);
            ++mNavMeshRevision;
        }
        return UpdateNavMeshStatusBuilder().removed(removed).getResult();
    }
}
