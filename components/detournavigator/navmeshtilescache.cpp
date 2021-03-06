#include "navmeshtilescache.hpp"

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

        const auto tile = mValues.find(std::make_tuple(agentHalfExtents, changedTile, NavMeshKeyView(recastMesh, offMeshConnections)));
        if (tile == mValues.end())
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
        const auto emplaced = mValues.emplace(std::make_tuple(agentHalfExtents, changedTile, NavMeshKeyRef(iterator->mNavMeshKey)), iterator);

        if (!emplaced.second)
        {
            mFreeItems.erase(iterator);
            acquireItemUnsafe(emplaced.first->second);
            ++mGetCount;
            ++mHitCount;
            return Value(*this, emplaced.first->second);
        }

        iterator->mNavMeshData = std::move(value);
        ++iterator->mUseCount;
        mUsedNavMeshDataSize += itemSize;
        mBusyItems.splice(mBusyItems.end(), mFreeItems, iterator);

        return Value(*this, iterator);
    }

    NavMeshTilesCache::Stats NavMeshTilesCache::getStats() const
    {
        Stats result;
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            result.mNavMeshCacheSize = mUsedNavMeshDataSize;
            result.mUsedNavMeshTiles = mBusyItems.size();
            result.mCachedNavMeshTiles = mFreeItems.size();
            result.mHitCount = mHitCount;
            result.mGetCount = mGetCount;
        }
        return result;
    }

    void NavMeshTilesCache::reportStats(unsigned int frameNumber, osg::Stats& out) const
    {
        const Stats stats = getStats();
        out.setAttribute(frameNumber, "NavMesh CacheSize", stats.mNavMeshCacheSize);
        out.setAttribute(frameNumber, "NavMesh UsedTiles", stats.mUsedNavMeshTiles);
        out.setAttribute(frameNumber, "NavMesh CachedTiles", stats.mCachedNavMeshTiles);
        out.setAttribute(frameNumber, "NavMesh CacheHitRate", static_cast<double>(stats.mHitCount) / stats.mGetCount * 100.0);
    }

    void NavMeshTilesCache::removeLeastRecentlyUsed()
    {
        const auto& item = mFreeItems.back();

        const auto value = mValues.find(std::make_tuple(item.mAgentHalfExtents, item.mChangedTile, NavMeshKeyRef(item.mNavMeshKey)));
        if (value == mValues.end())
            return;

        mUsedNavMeshDataSize -= item.mSize;
        mFreeNavMeshDataSize -= item.mSize;

        mValues.erase(value);
        mFreeItems.pop_back();
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
