#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H

#include "settings.hpp"
#include "settingsutils.hpp"
#include "tileposition.hpp"
#include "objectid.hpp"
#include "offmeshconnection.hpp"

#include <components/misc/guarded.hpp>

#include <osg/Vec3f>

#include <algorithm>
#include <map>
#include <unordered_set>
#include <vector>
#include <set>

namespace DetourNavigator
{
    class OffMeshConnectionsManager
    {
    public:
        OffMeshConnectionsManager(const Settings& settings)
            : mSettings(settings)
        {}

        void add(const ObjectId id, const OffMeshConnection& value)
        {
            const auto values = mValues.lock();

            values->mById.insert(std::make_pair(id, value));

            const auto startTilePosition = getTilePosition(mSettings, value.mStart);
            const auto endTilePosition = getTilePosition(mSettings, value.mEnd);

            values->mByTilePosition[startTilePosition].insert(id);

            if (startTilePosition != endTilePosition)
                values->mByTilePosition[endTilePosition].insert(id);
        }

        std::set<TilePosition> remove(const ObjectId id)
        {
            const auto values = mValues.lock();

            const auto byId = values->mById.equal_range(id);

            if (byId.first == byId.second) {
                return {};
            }

            std::set<TilePosition> removed;

            std::for_each(byId.first, byId.second, [&] (const auto& v) {
                const auto startTilePosition = getTilePosition(mSettings, v.second.mStart);
                const auto endTilePosition = getTilePosition(mSettings, v.second.mEnd);

                removed.emplace(startTilePosition);
                if (startTilePosition != endTilePosition)
                    removed.emplace(endTilePosition);
            });

            values->mById.erase(byId.first, byId.second);

            return removed;
        }

        std::vector<OffMeshConnection> get(const TilePosition& tilePosition)
        {
            std::vector<OffMeshConnection> result;

            const auto values = mValues.lock();

            const auto itByTilePosition = values->mByTilePosition.find(tilePosition);

            if (itByTilePosition == values->mByTilePosition.end())
                return result;

            std::for_each(itByTilePosition->second.begin(), itByTilePosition->second.end(),
                [&] (const ObjectId v)
                {
                    const auto byId = values->mById.equal_range(v);
                    std::for_each(byId.first, byId.second, [&] (const auto& v) { result.push_back(v.second); });
                });

            return result;
        }

    private:
        struct Values
        {
            std::multimap<ObjectId, OffMeshConnection> mById;
            std::map<TilePosition, std::unordered_set<ObjectId>> mByTilePosition;
        };

        const Settings& mSettings;
        Misc::ScopeGuarded<Values> mValues;

        void removeByTilePosition(std::map<TilePosition, std::unordered_set<ObjectId>>& valuesByTilePosition,
            const TilePosition& tilePosition, const ObjectId id)
        {
            const auto it = valuesByTilePosition.find(tilePosition);
            if (it != valuesByTilePosition.end())
                it->second.erase(id);
        }
    };
}

#endif
