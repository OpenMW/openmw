#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H

#include "settings.hpp"
#include "settingsutils.hpp"
#include "tileposition.hpp"
#include "objectid.hpp"
#include "offmeshconnection.hpp"

#include <components/misc/guarded.hpp>

#include <osg/Vec3f>

#include <boost/optional.hpp>

#include <algorithm>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace DetourNavigator
{
    class OffMeshConnectionsManager
    {
    public:
        OffMeshConnectionsManager(const Settings& settings)
            : mSettings(settings)
        {}

        bool add(const ObjectId id, const OffMeshConnection& value)
        {
            const auto values = mValues.lock();

            if (!values->mById.insert(std::make_pair(id, value)).second)
                return false;

            const auto startTilePosition = getTilePosition(mSettings, value.mStart);
            const auto endTilePosition = getTilePosition(mSettings, value.mEnd);

            values->mByTilePosition[startTilePosition].insert(id);

            if (startTilePosition != endTilePosition)
                values->mByTilePosition[endTilePosition].insert(id);

            return true;
        }

        boost::optional<OffMeshConnection> remove(const ObjectId id)
        {
            const auto values = mValues.lock();

            const auto itById = values->mById.find(id);

            if (itById == values->mById.end())
                return boost::none;

            const auto result = itById->second;

            values->mById.erase(itById);

            const auto startTilePosition = getTilePosition(mSettings, result.mStart);
            const auto endTilePosition = getTilePosition(mSettings, result.mEnd);

            removeByTilePosition(values->mByTilePosition, startTilePosition, id);

            if (startTilePosition != endTilePosition)
                removeByTilePosition(values->mByTilePosition, endTilePosition, id);

            return result;
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
                    const auto itById = values->mById.find(v);
                    if (itById != values->mById.end())
                        result.push_back(itById->second);
                });

            return result;
        }

    private:
        struct Values
        {
            std::unordered_map<ObjectId, OffMeshConnection> mById;
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
