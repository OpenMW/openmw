#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHTILESCACHE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHTILESCACHE_H

#include "offmeshconnection.hpp"
#include "navmeshdata.hpp"
#include "recastmesh.hpp"
#include "tileposition.hpp"

#include <atomic>
#include <map>
#include <list>
#include <mutex>

namespace DetourNavigator
{
    struct NavMeshDataRef
    {
        unsigned char* mValue;
        int mSize;
    };

    class NavMeshTilesCache
    {
    public:
        struct Item
        {
            std::atomic<std::int64_t> mUseCount;
            osg::Vec3f mAgentHalfExtents;
            TilePosition mChangedTile;
            std::string mNavMeshKey;
            NavMeshData mNavMeshData;

            Item(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile, std::string navMeshKey)
                : mUseCount(0)
                , mAgentHalfExtents(agentHalfExtents)
                , mChangedTile(changedTile)
                , mNavMeshKey(std::move(navMeshKey))
            {}
        };

        using ItemIterator = std::list<Item>::iterator;

        class Value
        {
        public:
            Value()
                : mOwner(nullptr), mIterator() {}

            Value(NavMeshTilesCache& owner, ItemIterator iterator)
                : mOwner(&owner), mIterator(iterator)
            {
            }

            Value(const Value& other) = delete;

            Value(Value&& other)
                : mOwner(other.mOwner), mIterator(other.mIterator)
            {
                other.mIterator = ItemIterator();
            }

            ~Value()
            {
                if (mIterator != ItemIterator())
                    mOwner->releaseItem(mIterator);
            }

            Value& operator =(const Value& other) = delete;

            Value& operator =(Value&& other)
            {
                if (mIterator == other.mIterator)
                    return *this;

                if (mIterator != ItemIterator())
                    mOwner->releaseItem(mIterator);

                mOwner = other.mOwner;
                mIterator = other.mIterator;

                other.mIterator = ItemIterator();

                return *this;
            }

            NavMeshDataRef get() const
            {
                return NavMeshDataRef {mIterator->mNavMeshData.mValue.get(), mIterator->mNavMeshData.mSize};
            }

            operator bool() const
            {
                return mIterator != ItemIterator();
            }

        private:
            NavMeshTilesCache* mOwner;
            ItemIterator mIterator;
        };

        NavMeshTilesCache(const std::size_t maxNavMeshDataSize);

        Value get(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
            const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections);

        Value set(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
            const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections,
            NavMeshData&& value);

    private:

        struct TileMap
        {
            std::map<std::string, ItemIterator> Map;
        };

        std::mutex mMutex;
        std::size_t mMaxNavMeshDataSize;
        std::size_t mUsedNavMeshDataSize;
        std::size_t mFreeNavMeshDataSize;
        std::list<Item> mBusyItems;
        std::list<Item> mFreeItems;
        std::map<osg::Vec3f, std::map<TilePosition, TileMap>> mValues;

        void removeLeastRecentlyUsed();

        void acquireItemUnsafe(ItemIterator iterator);

        void releaseItem(ItemIterator iterator);

        static std::size_t getSize(const Item& item)
        {
            return static_cast<std::size_t>(item.mNavMeshData.mSize) + 2 * item.mNavMeshKey.size();
        }
    };
}

#endif
