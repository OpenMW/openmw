#include "navmeshtilescache.hpp"
#include "exceptions.hpp"

#include <iostream>

namespace DetourNavigator
{
    namespace
    {
        inline std::string makeNavMeshKey(const RecastMesh& recastMesh,
            const std::vector<OffMeshConnection>& offMeshConnections)
        {
            std::string result;
            result.reserve(
                recastMesh.getIndices().size() * sizeof(int)
                + recastMesh.getVertices().size() * sizeof(float)
                + recastMesh.getAreaTypes().size() * sizeof(AreaType)
                + recastMesh.getWater().size() * sizeof(RecastMesh::Water)
                + offMeshConnections.size() * sizeof(OffMeshConnection)
            );
            std::copy(
                reinterpret_cast<const char*>(recastMesh.getIndices().data()),
                reinterpret_cast<const char*>(recastMesh.getIndices().data() + recastMesh.getIndices().size()),
                std::back_inserter(result)
            );
            std::copy(
                reinterpret_cast<const char*>(recastMesh.getVertices().data()),
                reinterpret_cast<const char*>(recastMesh.getVertices().data() + recastMesh.getVertices().size()),
                std::back_inserter(result)
            );
            std::copy(
                reinterpret_cast<const char*>(recastMesh.getAreaTypes().data()),
                reinterpret_cast<const char*>(recastMesh.getAreaTypes().data() + recastMesh.getAreaTypes().size()),
                std::back_inserter(result)
            );
            std::copy(
                reinterpret_cast<const char*>(recastMesh.getWater().data()),
                reinterpret_cast<const char*>(recastMesh.getWater().data() + recastMesh.getWater().size()),
                std::back_inserter(result)
            );
            std::copy(
                reinterpret_cast<const char*>(offMeshConnections.data()),
                reinterpret_cast<const char*>(offMeshConnections.data() + offMeshConnections.size()),
                std::back_inserter(result)
            );
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

        // TODO: use different function to make key to avoid unnecessary std::string allocation
        const auto tile = tileValues->second.Map.find(makeNavMeshKey(recastMesh, offMeshConnections));
        if (tile == tileValues->second.Map.end())
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

        const auto navMeshKey = makeNavMeshKey(recastMesh, offMeshConnections);
        const auto itemSize = navMeshSize + 2 * navMeshKey.size();

        if (itemSize > mFreeNavMeshDataSize + (mMaxNavMeshDataSize - mUsedNavMeshDataSize))
            return Value();

        while (!mFreeItems.empty() && mUsedNavMeshDataSize + itemSize > mMaxNavMeshDataSize)
            removeLeastRecentlyUsed();

        const auto iterator = mFreeItems.emplace(mFreeItems.end(), agentHalfExtents, changedTile, navMeshKey);
        // TODO: use std::string_view or some alternative to avoid navMeshKey copy into both mFreeItems and mValues
        const auto emplaced = mValues[agentHalfExtents][changedTile].Map.emplace(navMeshKey, iterator);

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

    void NavMeshTilesCache::removeLeastRecentlyUsed()
    {
        const auto& item = mFreeItems.back();

        const auto agentValues = mValues.find(item.mAgentHalfExtents);
        if (agentValues == mValues.end())
            return;

        const auto tileValues = agentValues->second.find(item.mChangedTile);
        if (tileValues == agentValues->second.end())
            return;

        const auto value = tileValues->second.Map.find(item.mNavMeshKey);
        if (value == tileValues->second.Map.end())
            return;

        mUsedNavMeshDataSize -= getSize(item);
        mFreeNavMeshDataSize -= getSize(item);
        mFreeItems.pop_back();

        tileValues->second.Map.erase(value);
        if (!tileValues->second.Map.empty())
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
}
