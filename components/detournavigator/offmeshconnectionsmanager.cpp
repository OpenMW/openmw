#include "offmeshconnectionsmanager.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "tileposition.hpp"
#include "objectid.hpp"
#include "offmeshconnection.hpp"

#include <algorithm>
#include <vector>
#include <set>

namespace DetourNavigator
{
    OffMeshConnectionsManager::OffMeshConnectionsManager(const Settings& settings)
        : mSettings(settings)
    {}

    void OffMeshConnectionsManager::add(const ObjectId id, const OffMeshConnection& value)
    {
        const auto values = mValues.lock();

        values->mById.insert(std::make_pair(id, value));

        const auto startTilePosition = getTilePosition(mSettings, value.mStart);
        const auto endTilePosition = getTilePosition(mSettings, value.mEnd);

        values->mByTilePosition[startTilePosition].insert(id);

        if (startTilePosition != endTilePosition)
            values->mByTilePosition[endTilePosition].insert(id);
    }

    std::set<TilePosition> OffMeshConnectionsManager::remove(const ObjectId id)
    {
        const auto values = mValues.lock();

        const auto byId = values->mById.equal_range(id);

        if (byId.first == byId.second)
            return {};

        std::set<TilePosition> removed;

        std::for_each(byId.first, byId.second, [&] (const auto& v) {
            const auto startTilePosition = getTilePosition(mSettings, v.second.mStart);
            const auto endTilePosition = getTilePosition(mSettings, v.second.mEnd);

            removed.emplace(startTilePosition);
            if (startTilePosition != endTilePosition)
                removed.emplace(endTilePosition);
        });

        for (const TilePosition& tilePosition : removed)
        {
            const auto it = values->mByTilePosition.find(tilePosition);
            if (it == values->mByTilePosition.end())
                continue;
            it->second.erase(id);
            if (it->second.empty())
                values->mByTilePosition.erase(it);
        }

        values->mById.erase(byId.first, byId.second);

        return removed;
    }

    std::vector<OffMeshConnection> OffMeshConnectionsManager::get(const TilePosition& tilePosition)
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
                std::for_each(byId.first, byId.second, [&] (const auto& v)
                {
                    if (getTilePosition(mSettings, v.second.mStart) == tilePosition
                            || getTilePosition(mSettings, v.second.mEnd) == tilePosition)
                        result.push_back(v.second);
                });
            });

        std::sort(result.begin(), result.end());

        return result;
    }
}
