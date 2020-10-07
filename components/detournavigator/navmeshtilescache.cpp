#include "navmeshtilescache.hpp"
#include "exceptions.hpp"

#include <osg/Stats>

#include <cstring>

namespace DetourNavigator
{
    namespace
    {
        inline std::vector<unsigned char> makeNavMeshKey(const RecastMesh& recastMesh,
            const std::vector<OffMeshConnection>& offMeshConnections)
        {
            const std::size_t indicesSize = recastMesh.getIndices().size() * sizeof(int);
            const std::size_t verticesSize = recastMesh.getVertices().size() * sizeof(float);
            const std::size_t areaTypesSize = recastMesh.getAreaTypes().size() * sizeof(AreaType);
            const std::size_t waterSize = recastMesh.getWater().size() * sizeof(RecastMesh::Water);
            const std::size_t offMeshConnectionsSize = offMeshConnections.size() * sizeof(OffMeshConnection);

            std::vector<unsigned char> result(indicesSize + verticesSize + areaTypesSize + waterSize + offMeshConnectionsSize);
            unsigned char* dst = result.data();

            std::memcpy(dst, recastMesh.getIndices().data(), indicesSize);
            dst += indicesSize;

            std::memcpy(dst, recastMesh.getVertices().data(), verticesSize);
            dst += verticesSize;

            std::memcpy(dst, recastMesh.getAreaTypes().data(), areaTypesSize);
            dst += areaTypesSize;

            std::memcpy(dst, recastMesh.getWater().data(), waterSize);
            dst += waterSize;

            std::memcpy(dst, offMeshConnections.data(), offMeshConnectionsSize);

            return result;
        }
    }

    NavMeshTilesCache::NavMeshTilesCache(const std::size_t maxNavMeshDataSize)
        : mMaxNavMeshDataSize(maxNavMeshDataSize), mUsedNavMeshDataSize(0), mFreeNavMeshDataSize(0) {}

    NavMeshTilesCache::Value NavMeshTilesCache::get(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
        const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections)
    {
        const std::lock_guard<std::mutex> lock(mMutex);

        const auto agentValues = mValues.find(agentHalfExtents);
        if (agentValues == mValues.end())
            return Value();

        const auto tileValues = agentValues->second.find(changedTile);
        if (tileValues == agentValues->second.end())
            return Value();

        const auto tile = tileValues->second.mMap.find(RecastMeshKeyView(recastMesh, offMeshConnections));
        if (tile == tileValues->second.mMap.end())
            return Value();

        acquireItemUnsafe(tile->second);

        return Value(*this, tile->second);
    }

    NavMeshTilesCache::Value NavMeshTilesCache::set(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
        const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections,
        NavMeshData&& value)
    {
        const auto navMeshSize = static_cast<std::size_t>(value.mSize);

        const std::lock_guard<std::mutex> lock(mMutex);

        if (navMeshSize > mMaxNavMeshDataSize)
            return Value();

        if (navMeshSize > mFreeNavMeshDataSize + (mMaxNavMeshDataSize - mUsedNavMeshDataSize))
            return Value();

        auto navMeshKey = makeNavMeshKey(recastMesh, offMeshConnections);
        const auto itemSize = navMeshSize + 2 * navMeshKey.size();

        if (itemSize > mFreeNavMeshDataSize + (mMaxNavMeshDataSize - mUsedNavMeshDataSize))
            return Value();

        while (!mFreeItems.empty() && mUsedNavMeshDataSize + itemSize > mMaxNavMeshDataSize)
            removeLeastRecentlyUsed();

        const auto iterator = mFreeItems.emplace(mFreeItems.end(), agentHalfExtents, changedTile, std::move(navMeshKey));
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

        {
            const std::lock_guard<std::mutex> lock(mMutex);
            navMeshCacheSize = mUsedNavMeshDataSize;
            usedNavMeshTiles = mBusyItems.size();
            cachedNavMeshTiles = mFreeItems.size();
        }

        stats.setAttribute(frameNumber, "NavMesh CacheSize", navMeshCacheSize);
        stats.setAttribute(frameNumber, "NavMesh UsedTiles", usedNavMeshTiles);
        stats.setAttribute(frameNumber, "NavMesh CachedTiles", cachedNavMeshTiles);
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

        mUsedNavMeshDataSize -= getSize(item);
        mFreeNavMeshDataSize -= getSize(item);

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
        mFreeNavMeshDataSize -= getSize(*iterator);
    }

    void NavMeshTilesCache::releaseItem(ItemIterator iterator)
    {
        if (--iterator->mUseCount > 0)
            return;

        const std::lock_guard<std::mutex> lock(mMutex);

        mFreeItems.splice(mFreeItems.begin(), mBusyItems, iterator);
        mFreeNavMeshDataSize += getSize(*iterator);
    }

    namespace
    {
        struct CompareBytes
        {
            const unsigned char* mRhsIt;
            const unsigned char* const mRhsEnd;

            template <class T>
            int operator ()(const std::vector<T>& lhs)
            {
                const auto lhsBegin = reinterpret_cast<const char*>(lhs.data());
                const auto lhsEnd = reinterpret_cast<const char*>(lhs.data() + lhs.size());
                const auto lhsSize = static_cast<std::ptrdiff_t>(lhsEnd - lhsBegin);
                const auto rhsSize = static_cast<std::ptrdiff_t>(mRhsEnd - mRhsIt);

                if (lhsBegin == nullptr || mRhsIt == nullptr)
                {
                    if (lhsSize < rhsSize)
                        return -1;
                    else if (lhsSize > rhsSize)
                        return 1;
                    else
                        return 0;
                }

                const auto size = std::min(lhsSize, rhsSize);

                if (const auto result = std::memcmp(lhsBegin, mRhsIt, size))
                    return result;

                if (lhsSize > rhsSize)
                    return 1;

                mRhsIt += size;

                return 0;
            }
        };
    }

    int NavMeshTilesCache::RecastMeshKeyView::compare(const std::vector<unsigned char>& other) const
    {
        CompareBytes compareBytes {other.data(), other.data() + other.size()};

        if (const auto result = compareBytes(mRecastMesh.get().getIndices()))
            return result;

        if (const auto result = compareBytes(mRecastMesh.get().getVertices()))
            return result;

        if (const auto result = compareBytes(mRecastMesh.get().getAreaTypes()))
            return result;

        if (const auto result = compareBytes(mRecastMesh.get().getWater()))
            return result;

        if (const auto result = compareBytes(mOffMeshConnections.get()))
            return result;

        if (compareBytes.mRhsIt < compareBytes.mRhsEnd)
            return -1;

        return 0;
    }
}
