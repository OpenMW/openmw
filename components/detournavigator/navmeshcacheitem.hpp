#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHCACHEITEM_H

#include "sharednavmesh.hpp"
#include "tileposition.hpp"
#include "navmeshtilescache.hpp"
#include "dtstatus.hpp"
#include "navmeshdata.hpp"

#include <components/misc/guarded.hpp>

#include <map>
#include <ostream>

struct dtMeshTile;

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

    inline std::ostream& operator <<(std::ostream& stream, UpdateNavMeshStatus value)
    {
        switch (value)
        {
            case UpdateNavMeshStatus::ignored:
                return stream << "ignore";
            case UpdateNavMeshStatus::removed:
                return stream << "removed";
            case UpdateNavMeshStatus::added:
                return stream << "add";
            case UpdateNavMeshStatus::replaced:
                return stream << "replaced";
            case UpdateNavMeshStatus::failed:
                return stream << "failed";
            case UpdateNavMeshStatus::lost:
                return stream << "lost";
            case UpdateNavMeshStatus::cached:
                return stream << "cached";
            case UpdateNavMeshStatus::unchanged:
                return stream << "unchanged";
            case UpdateNavMeshStatus::restored:
                return stream << "restored";
        }
        return stream << "unknown(" << static_cast<unsigned>(value) << ")";
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

        UpdateNavMeshStatus updateTile(const TilePosition& position, NavMeshTilesCache::Value&& cached,
                                       NavMeshData&& navMeshData);

        UpdateNavMeshStatus removeTile(const TilePosition& position);

    private:
        NavMeshPtr mImpl;
        std::size_t mGeneration;
        std::size_t mNavMeshRevision;
        std::map<TilePosition, std::pair<NavMeshTilesCache::Value, NavMeshData>> mUsedTiles;
    };

    using GuardedNavMeshCacheItem = Misc::ScopeGuarded<NavMeshCacheItem>;
    using SharedNavMeshCacheItem = std::shared_ptr<GuardedNavMeshCacheItem>;
}

#endif
