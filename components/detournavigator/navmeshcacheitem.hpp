#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H

#include "navmeshdata.hpp"
#include "navmeshtilescache.hpp"
#include "tileposition.hpp"
#include "version.hpp"

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <iosfwd>
#include <map>
#include <set>

struct dtMeshTile;

namespace DetourNavigator
{
    struct Settings;

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

    std::ostream& operator<<(std::ostream& stream, UpdateNavMeshStatus value);

    class UpdateNavMeshStatusBuilder
    {
    public:
        UpdateNavMeshStatusBuilder() = default;

        explicit UpdateNavMeshStatusBuilder(UpdateNavMeshStatus value)
            : mResult(value)
        {
        }

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

        UpdateNavMeshStatus getResult() const { return mResult; }

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

    const dtMeshTile* getTile(const dtNavMesh& navMesh, const TilePosition& position);

    class NavMeshCacheItem
    {
    public:
        explicit NavMeshCacheItem(std::size_t generation, const Settings& settings);

        const dtNavMesh& getImpl() const { return mImpl; }

        dtNavMeshQuery& getQuery() { return mQuery; }

        const Version& getVersion() const { return mVersion; }

        UpdateNavMeshStatus updateTile(
            const TilePosition& position, NavMeshTilesCache::Value&& cached, NavMeshData&& navMeshData);

        UpdateNavMeshStatus removeTile(const TilePosition& position);

        UpdateNavMeshStatus markAsEmpty(const TilePosition& position);

        bool isEmptyTile(const TilePosition& position) const;

        template <class Function>
        void forEachUsedTile(Function&& function) const
        {
            for (const auto& [position, tile] : mUsedTiles)
                if (const dtMeshTile* meshTile = getTile(mImpl, position))
                    function(position, tile.mVersion, *meshTile);
        }

        template <class Function>
        void forEachTilePosition(Function&& function) const
        {
            for (const auto& [position, tile] : mUsedTiles)
                function(position);
            for (const TilePosition& position : mEmptyTiles)
                function(position);
        }

    private:
        struct Tile
        {
            Version mVersion;
            NavMeshTilesCache::Value mCached;
            NavMeshData mData;
        };

        Version mVersion;
        dtNavMesh mImpl;
        dtNavMeshQuery mQuery;
        std::map<TilePosition, Tile> mUsedTiles;
        std::set<TilePosition> mEmptyTiles;
    };
}

#endif
