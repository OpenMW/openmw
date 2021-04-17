#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H

#include "sharednavmesh.hpp"
#include "tileposition.hpp"
#include "navmeshtilescache.hpp"
#include "dtstatus.hpp"
#include "navmeshtileview.hpp"

#include <components/misc/guarded.hpp>

#include <DetourNavMesh.h>

#include <map>

namespace DetourNavigator
{
    enum class UpdateNavMeshStatus : unsigned
    {
        ignored = 0,
        removed = 1 << 0,
        added = 1 << 1,
        replaced = removed | added,
        failed = 1 << 2,
        lost = removed | failed,
        cached = 1 << 3,
        unchanged = replaced | cached,
        restored = added | cached,
    };

    inline bool isSuccess(UpdateNavMeshStatus value)
    {
        return (static_cast<unsigned>(value) & static_cast<unsigned>(UpdateNavMeshStatus::failed)) == 0;
    }

    class UpdateNavMeshStatusBuilder
    {
    public:
        UpdateNavMeshStatusBuilder() = default;

        explicit UpdateNavMeshStatusBuilder(UpdateNavMeshStatus value)
            : mResult(value) {}

        UpdateNavMeshStatusBuilder removed(bool value)
        {
            if (value)
                set(UpdateNavMeshStatus::removed);
            else
                unset(UpdateNavMeshStatus::removed);
            return *this;
        }

        UpdateNavMeshStatusBuilder added(bool value)
        {
            if (value)
                set(UpdateNavMeshStatus::added);
            else
                unset(UpdateNavMeshStatus::added);
            return *this;
        }

        UpdateNavMeshStatusBuilder failed(bool value)
        {
            if (value)
                set(UpdateNavMeshStatus::failed);
            else
                unset(UpdateNavMeshStatus::failed);
            return *this;
        }

        UpdateNavMeshStatusBuilder cached(bool value)
        {
            if (value)
                set(UpdateNavMeshStatus::cached);
            else
                unset(UpdateNavMeshStatus::cached);
            return *this;
        }

        UpdateNavMeshStatus getResult() const
        {
            return mResult;
        }

    private:
        UpdateNavMeshStatus mResult = UpdateNavMeshStatus::ignored;

        void set(UpdateNavMeshStatus value)
        {
            mResult = static_cast<UpdateNavMeshStatus>(static_cast<unsigned>(mResult) | static_cast<unsigned>(value));
        }

        void unset(UpdateNavMeshStatus value)
        {
            mResult = static_cast<UpdateNavMeshStatus>(static_cast<unsigned>(mResult) & ~static_cast<unsigned>(value));
        }
    };

    inline unsigned char* getRawData(NavMeshData& navMeshData)
    {
        return navMeshData.mValue.get();
    }

    inline unsigned char* getRawData(NavMeshTilesCache::Value& cachedNavMeshData)
    {
        return cachedNavMeshData.get().mValue;
    }

    inline int getSize(const NavMeshData& navMeshData)
    {
        return navMeshData.mSize;
    }

    inline int getSize(const NavMeshTilesCache::Value& cachedNavMeshData)
    {
        return cachedNavMeshData.get().mSize;
    }

    class NavMeshCacheItem
    {
    public:
        NavMeshCacheItem(const NavMeshPtr& impl, std::size_t generation)
            : mImpl(impl), mGeneration(generation), mNavMeshRevision(0)
        {
        }

        const dtNavMesh& getImpl() const
        {
            return *mImpl;
        }

        std::size_t getGeneration() const
        {
            return mGeneration;
        }

        std::size_t getNavMeshRevision() const
        {
            return mNavMeshRevision;
        }

        template <class T>
        UpdateNavMeshStatus updateTile(const TilePosition& position, T&& navMeshData)
        {
            const dtMeshTile* currentTile = getTile(position);
            if (currentTile != nullptr
                && asNavMeshTileConstView(*currentTile) == asNavMeshTileConstView(getRawData(navMeshData)))
            {
                return UpdateNavMeshStatus::ignored;
            }
            const auto removed = removeTileImpl(position);
            const auto addStatus = addTileImpl(getRawData(navMeshData), getSize(navMeshData));
            if (dtStatusSucceed(addStatus))
            {
                setUsedTile(position, std::forward<T>(navMeshData));
                return UpdateNavMeshStatusBuilder().added(true).removed(removed).getResult();
            }
            else
            {
                if (removed)
                    removeUsedTile(position);
                return UpdateNavMeshStatusBuilder().removed(removed).failed((addStatus & DT_OUT_OF_MEMORY) != 0).getResult();
            }
        }

        UpdateNavMeshStatus removeTile(const TilePosition& position)
        {
            const auto removed = removeTileImpl(position);
            if (removed)
                removeUsedTile(position);
            return UpdateNavMeshStatusBuilder().removed(removed).getResult();
        }

    private:
        NavMeshPtr mImpl;
        std::size_t mGeneration;
        std::size_t mNavMeshRevision;
        std::map<TilePosition, std::pair<NavMeshTilesCache::Value, NavMeshData>> mUsedTiles;

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

        dtStatus addTileImpl(unsigned char* data, int size)
        {
            const int doNotTransferOwnership = 0;
            const dtTileRef lastRef = 0;
            dtTileRef* const result = nullptr;
            return mImpl->addTile(data, size, doNotTransferOwnership, lastRef, result);
        }

        bool removeTileImpl(const TilePosition& position)
        {
            const int layer = 0;
            const auto tileRef = mImpl->getTileRefAt(position.x(), position.y(), layer);
            if (tileRef == 0)
                return false;
            unsigned char** const data = nullptr;
            int* const dataSize = nullptr;
            return dtStatusSucceed(mImpl->removeTile(tileRef, data, dataSize));
        }

        const dtMeshTile* getTile(const TilePosition& position) const
        {
            const int layer = 0;
            return mImpl->getTileAt(position.x(), position.y(), layer);
        }
    };

    using GuardedNavMeshCacheItem = Misc::ScopeGuarded<NavMeshCacheItem>;
    using SharedNavMeshCacheItem = std::shared_ptr<GuardedNavMeshCacheItem>;
}

#endif
