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
    const dtMeshTile* getTile(const dtNavMesh& navMesh, const TilePosition& position)
    {
        const int layer = 0;
        return navMesh.getTileAt(position.x(), position.y(), layer);
    }

    UpdateNavMeshStatus NavMeshCacheItem::updateTile(const TilePosition& position, NavMeshTilesCache::Value&& cached,
        NavMeshData&& navMeshData)
    {
        const dtMeshTile* currentTile = getTile(*mImpl, position);
        if (currentTile != nullptr
            && asNavMeshTileConstView(*currentTile) == asNavMeshTileConstView(navMeshData.mValue.get()))
        {
            return UpdateNavMeshStatus::ignored;
        }
        bool removed = ::removeTile(*mImpl, position);
        removed = mEmptyTiles.erase(position) > 0 || removed;
        const auto addStatus = addTile(*mImpl, navMeshData.mValue.get(), navMeshData.mSize);
        if (dtStatusSucceed(addStatus))
        {
            auto tile = mUsedTiles.find(position);
            if (tile == mUsedTiles.end())
            {
                mUsedTiles.emplace_hint(tile, position,
                    Tile {Version {mVersion.mRevision, 1}, std::move(cached), std::move(navMeshData)});
            }
            else
            {
                ++tile->second.mVersion.mRevision;
                tile->second.mCached = std::move(cached);
                tile->second.mData = std::move(navMeshData);
            }
            ++mVersion.mRevision;
            return UpdateNavMeshStatusBuilder().added(true).removed(removed).getResult();
        }
        else
        {
            if (removed)
            {
                mUsedTiles.erase(position);
                ++mVersion.mRevision;
            }
            return UpdateNavMeshStatusBuilder().removed(removed).failed((addStatus & DT_OUT_OF_MEMORY) != 0).getResult();
        }
    }

    UpdateNavMeshStatus NavMeshCacheItem::removeTile(const TilePosition& position)
    {
        bool removed = ::removeTile(*mImpl, position);
        removed = mEmptyTiles.erase(position) > 0 || removed;
        if (removed)
        {
            mUsedTiles.erase(position);
            ++mVersion.mRevision;
        }
        return UpdateNavMeshStatusBuilder().removed(removed).getResult();
    }

    UpdateNavMeshStatus NavMeshCacheItem::markAsEmpty(const TilePosition& position)
    {
        bool removed = ::removeTile(*mImpl, position);
        removed = mEmptyTiles.insert(position).second || removed;
        if (removed)
        {
            mUsedTiles.erase(position);
            ++mVersion.mRevision;
        }
        return UpdateNavMeshStatusBuilder().removed(removed).getResult();
    }

    bool NavMeshCacheItem::isEmptyTile(const TilePosition& position) const
    {
        return mEmptyTiles.find(position) != mEmptyTiles.end();
    }
}
