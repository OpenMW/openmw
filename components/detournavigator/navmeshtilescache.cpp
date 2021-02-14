#include "navmeshtilescache.hpp"
#include "exceptions.hpp"

#include <osg/Stats>

#include <cstring>

namespace DetourNavigator
{
    namespace
    {
        inline std::size_t getSize(const RecastMesh& recastMesh,
                                   const std::vector<OffMeshConnection>& offMeshConnections)
        {
            const std::size_t indicesSize = recastMesh.getIndices().size() * sizeof(int);
            const std::size_t verticesSize = recastMesh.getVertices().size() * sizeof(float);
            const std::size_t areaTypesSize = recastMesh.getAreaTypes().size() * sizeof(AreaType);
            const std::size_t waterSize = recastMesh.getWater().size() * sizeof(RecastMesh::Water);
            const std::size_t offMeshConnectionsSize = offMeshConnections.size() * sizeof(OffMeshConnection);
            return indicesSize + verticesSize + areaTypesSize + waterSize + offMeshConnectionsSize;
        }
    }

    NavMeshTilesCache::NavMeshTilesCache(const std::size_t maxNavMeshDataSize)
        : mMaxNavMeshDataSize(maxNavMeshDataSize), mUsedNavMeshDataSize(0), mFreeNavMeshDataSize(0),
          mHitCount(0), mGetCount(0) {}

    NavMeshTilesCache::Value NavMeshTilesCache::get(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
        const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections)
    {
        const std::lock_guard<std::mutex> lock(mMutex);

        ++mGetCount;

        const auto agentValues = mValues.find(agentHalfExtents);
        if (agentValues == mValues.end())
            return Value();

        const auto tileValues = agentValues->second.find(changedTile);
        if (tileValues == agentValues->second.end())
            return Value();

        const auto tile = tileValues->second.mMap.find(NavMeshKeyView(recastMesh, offMeshConnections));
        if (tile == tileValues->second.mMap.end())
            return Value();

        acquireItemUnsafe(tile->second);

        ++mHitCount;

        return Value(*this, tile->second);
    }

    NavMeshTilesCache::Value NavMeshTilesCache::set(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
        const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections,
        NavMeshData&& value)
    {
        const auto itemSize = static_cast<std::size_t>(value.mSize) + getSize(recastMesh, offMeshConnections);

        const std::lock_guard<std::mutex> lock(mMutex);

        if (itemSize > mFreeNavMeshDataSize + (mMaxNavMeshDataSize - mUsedNavMeshDataSize))
            return Value();

        while (!mFreeItems.empty() && mUsedNavMeshDataSize + itemSize > mMaxNavMeshDataSize)
            removeLeastRecentlyUsed();

        NavMeshKey navMeshKey {
            RecastMeshData {recastMesh.getIndices(), recastMesh.getVertices(), recastMesh.getAreaTypes(), recastMesh.getWater()},
            offMeshConnections
        };

        const auto iterator = mFreeItems.emplace(mFreeItems.end(), agentHalfExtents, changedTile, std::move(navMeshKey), itemSize);
        const auto emplaced = mValues[agentHalfExtents][changedTile].mMap.emplace(iterator->mNavMeshKey, iterator);

        if (!emplaced.second)
        {
            mFreeItems.erase(iterator);
            throw InvalidArgument("Set existing cache value");
        }

        iterator->mNavMeshData = std::move(value);
        mUsedNavMeshDataSize += itemSize;
        mFreeNavMeshDataSize += itemSize;

        acquireItemUnsafe(iterator);

        return Value(*this, iterator);
    }

    void NavMeshTilesCache::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        std::size_t navMeshCacheSize = 0;
        std::size_t usedNavMeshTiles = 0;
        std::size_t cachedNavMeshTiles = 0;
        std::size_t hitCount = 0;
        std::size_t getCount = 0;

        {
            const std::lock_guard<std::mutex> lock(mMutex);
            navMeshCacheSize = mUsedNavMeshDataSize;
            usedNavMeshTiles = mBusyItems.size();
            cachedNavMeshTiles = mFreeItems.size();
            hitCount = mHitCount;
            getCount = mGetCount;
        }

        stats.setAttribute(frameNumber, "NavMesh CacheSize", navMeshCacheSize);
        stats.setAttribute(frameNumber, "NavMesh UsedTiles", usedNavMeshTiles);
        stats.setAttribute(frameNumber, "NavMesh CachedTiles", cachedNavMeshTiles);
        stats.setAttribute(frameNumber, "NavMesh CacheHitRate", static_cast<double>(hitCount) / getCount * 100.0);
    }

    void NavMeshTilesCache::removeLeastRecentlyUsed()
    {
        const auto& item = mFreeItems.back();

        const auto agentValues = mValues.find(item.mAgentHalfExtents);
        if (agentValues == mValues.end())
            return;

        const auto tileValues = agentValues->second.find(item.mChangedTile);
        if (tileValues == agentValues->second.end())
            return;

        const auto value = tileValues->second.mMap.find(item.mNavMeshKey);
        if (value == tileValues->second.mMap.end())
            return;

        mUsedNavMeshDataSize -= item.mSize;
        mFreeNavMeshDataSize -= item.mSize;

        tileValues->second.mMap.erase(value);
        mFreeItems.pop_back();

        if (!tileValues->second.mMap.empty())
            return;

        agentValues->second.erase(tileValues);
        if (!agentValues->second.empty())
            return;

        mValues.erase(agentValues);
    }

    void NavMeshTilesCache::acquireItemUnsafe(ItemIterator iterator)
    {
        if (++iterator->mUseCount > 1)
            return;

        mBusyItems.splice(mBusyItems.end(), mFreeItems, iterator);
        mFreeNavMeshDataSize -= iterator->mSize;
    }

    void NavMeshTilesCache::releaseItem(ItemIterator iterator)
    {
        if (--iterator->mUseCount > 0)
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

        mFreeItems.splice(mFreeItems.begin(), mBusyItems, iterator);
        mFreeNavMeshDataSize += iterator->mSize;
    }
}
