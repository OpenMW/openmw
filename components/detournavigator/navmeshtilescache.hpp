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
#include <cassert>
#include <cstring>
#include <vector>

namespace osg
{
    class Stats;
}

namespace DetourNavigator
{
    struct NavMeshDataRef
    {
        unsigned char* mValue;
        int mSize;
    };

    struct RecastMeshData
    {
        std::vector<int> mIndices;
        std::vector<float> mVertices;
        std::vector<AreaType> mAreaTypes;
        std::vector<RecastMesh::Water> mWater;
    };

    inline bool operator <(const RecastMeshData& lhs, const RecastMeshData& rhs)
    {
        return std::tie(lhs.mIndices, lhs.mVertices, lhs.mAreaTypes, lhs.mWater)
                < std::tie(rhs.mIndices, rhs.mVertices, rhs.mAreaTypes, rhs.mWater);
    }

    inline bool operator <(const RecastMeshData& lhs, const RecastMesh& rhs)
    {
        return std::tie(lhs.mIndices, lhs.mVertices, lhs.mAreaTypes, lhs.mWater)
                < std::tie(rhs.getIndices(), rhs.getVertices(), rhs.getAreaTypes(), rhs.getWater());
    }

    inline bool operator <(const RecastMesh& lhs, const RecastMeshData& rhs)
    {
        return std::tie(lhs.getIndices(), lhs.getVertices(), lhs.getAreaTypes(), lhs.getWater())
                < std::tie(rhs.mIndices, rhs.mVertices, rhs.mAreaTypes, rhs.mWater);
    }

    struct NavMeshKey
    {
        RecastMeshData mRecastMesh;
        std::vector<OffMeshConnection> mOffMeshConnections;
    };

    inline bool operator <(const NavMeshKey& lhs, const NavMeshKey& rhs)
    {
        return std::tie(lhs.mRecastMesh, lhs.mOffMeshConnections)
                < std::tie(rhs.mRecastMesh, rhs.mOffMeshConnections);
    }

    struct NavMeshKeyRef
    {
        std::reference_wrapper<const NavMeshKey> mRef;

        explicit NavMeshKeyRef(const NavMeshKey& ref) : mRef(ref) {}
    };

    inline bool operator <(const NavMeshKeyRef& lhs, const NavMeshKeyRef& rhs)
    {
        return lhs.mRef.get() < rhs.mRef.get();
    }

    struct NavMeshKeyView
    {
        std::reference_wrapper<const RecastMesh> mRecastMesh;
        std::reference_wrapper<const std::vector<OffMeshConnection>> mOffMeshConnections;

        NavMeshKeyView(const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections)
            : mRecastMesh(recastMesh), mOffMeshConnections(offMeshConnections) {}
    };

    inline bool operator <(const NavMeshKeyView& lhs, const NavMeshKey& rhs)
    {
        return std::tie(lhs.mRecastMesh.get(), lhs.mOffMeshConnections.get())
                < std::tie(rhs.mRecastMesh, rhs.mOffMeshConnections);
    }

    inline bool operator <(const NavMeshKey& lhs, const NavMeshKeyView& rhs)
    {
        return std::tie(lhs.mRecastMesh, lhs.mOffMeshConnections)
                < std::tie(rhs.mRecastMesh.get(), rhs.mOffMeshConnections.get());
    }

    template <class R>
    inline bool operator <(const NavMeshKeyRef& lhs, const R& rhs)
    {
        return lhs.mRef.get() < rhs;
    }

    template <class L>
    inline bool operator <(const L& lhs, const NavMeshKeyRef& rhs)
    {
        return lhs < rhs.mRef.get();
    }

    template <class L, class R>
    inline bool operator <(const std::tuple<osg::Vec3f, TilePosition, L>& lhs, const std::tuple<osg::Vec3f, TilePosition, R>& rhs)
    {
        const auto left = std::tie(std::get<0>(lhs), std::get<1>(lhs));
        const auto right = std::tie(std::get<0>(rhs), std::get<1>(rhs));
        return std::tie(left, std::get<2>(lhs)) < std::tie(right, std::get<2>(rhs));
    }

    class NavMeshTilesCache
    {
    public:
        struct Item
        {
            std::atomic<std::int64_t> mUseCount;
            osg::Vec3f mAgentHalfExtents;
            TilePosition mChangedTile;
            NavMeshKey mNavMeshKey;
            NavMeshData mNavMeshData;
            std::size_t mSize;

            Item(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile, NavMeshKey&& navMeshKey, std::size_t size)
                : mUseCount(0)
                , mAgentHalfExtents(agentHalfExtents)
                , mChangedTile(changedTile)
                , mNavMeshKey(navMeshKey)
                , mSize(size)
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
                other.mOwner = nullptr;
            }

            ~Value()
            {
                if (mOwner)
                    mOwner->releaseItem(mIterator);
            }

            Value& operator =(const Value& other) = delete;

            Value& operator =(Value&& other)
            {
                if (mOwner)
                    mOwner->releaseItem(mIterator);

                mOwner = other.mOwner;
                mIterator = other.mIterator;

                other.mOwner = nullptr;

                return *this;
            }

            NavMeshDataRef get() const
            {
                return NavMeshDataRef {mIterator->mNavMeshData.mValue.get(), mIterator->mNavMeshData.mSize};
            }

            operator bool() const
            {
                return mOwner;
            }

        private:
            NavMeshTilesCache* mOwner;
            ItemIterator mIterator;
        };

        struct Stats
        {
            std::size_t mNavMeshCacheSize;
            std::size_t mUsedNavMeshTiles;
            std::size_t mCachedNavMeshTiles;
            std::size_t mHitCount;
            std::size_t mGetCount;
        };

        NavMeshTilesCache(const std::size_t maxNavMeshDataSize);

        Value get(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
            const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections);

        Value set(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile,
            const RecastMesh& recastMesh, const std::vector<OffMeshConnection>& offMeshConnections,
            NavMeshData&& value);

        Stats getStats() const;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const;

    private:
        mutable std::mutex mMutex;
        std::size_t mMaxNavMeshDataSize;
        std::size_t mUsedNavMeshDataSize;
        std::size_t mFreeNavMeshDataSize;
        std::size_t mHitCount;
        std::size_t mGetCount;
        std::list<Item> mBusyItems;
        std::list<Item> mFreeItems;
        std::map<std::tuple<osg::Vec3f, TilePosition, NavMeshKeyRef>, ItemIterator, std::less<>> mValues;

        void removeLeastRecentlyUsed();

        void acquireItemUnsafe(ItemIterator iterator);

        void releaseItem(ItemIterator iterator);
    };
}

#endif
