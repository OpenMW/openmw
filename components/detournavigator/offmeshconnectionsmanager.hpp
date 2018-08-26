#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H

#include "settings.hpp"
#include "settingsutils.hpp"
#include "tileposition.hpp"

#include <osg/Vec3f>

#include <boost/optional.hpp>

#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace DetourNavigator
{
    struct OffMeshConnection
    {
        osg::Vec3f mStart;
        osg::Vec3f mEnd;
    };

    class OffMeshConnectionsManager
    {
    public:
        OffMeshConnectionsManager(const Settings& settings)
            : mSettings(settings)
        {}

        bool add(const std::size_t id, const OffMeshConnection& value)
        {
            const std::lock_guard<std::mutex> lock(mMutex);

            if (!mValuesById.insert(std::make_pair(id, value)).second)
                return false;

            const auto startTilePosition = getTilePosition(mSettings, value.mStart);
            const auto endTilePosition = getTilePosition(mSettings, value.mEnd);

            mValuesByTilePosition[startTilePosition].insert(id);

            if (startTilePosition != endTilePosition)
                mValuesByTilePosition[endTilePosition].insert(id);

            return true;
        }

        boost::optional<OffMeshConnection> remove(const std::size_t id)
        {
            const std::lock_guard<std::mutex> lock(mMutex);

            const auto itById = mValuesById.find(id);

            if (itById == mValuesById.end())
                return boost::none;

            const auto result = itById->second;

            mValuesById.erase(itById);

            const auto startTilePosition = getTilePosition(mSettings, result.mStart);
            const auto endTilePosition = getTilePosition(mSettings, result.mEnd);

            removeByTilePosition(startTilePosition, id);

            if (startTilePosition != endTilePosition)
                removeByTilePosition(endTilePosition, id);

            return result;
        }

        std::vector<OffMeshConnection> get(const TilePosition& tilePosition)
        {
            std::vector<OffMeshConnection> result;

            const std::lock_guard<std::mutex> lock(mMutex);

            const auto itByTilePosition = mValuesByTilePosition.find(tilePosition);

            if (itByTilePosition == mValuesByTilePosition.end())
                return result;

            std::for_each(itByTilePosition->second.begin(), itByTilePosition->second.end(),
                [&] (const std::size_t v)
                {
                    const auto itById = mValuesById.find(v);
                    if (itById != mValuesById.end())
                        result.push_back(itById->second);
                });

            return result;
        }

    private:
        const Settings& mSettings;
        std::mutex mMutex;
        std::unordered_map<std::size_t, OffMeshConnection> mValuesById;
        std::map<TilePosition, std::unordered_set<std::size_t>> mValuesByTilePosition;

        void removeByTilePosition(const TilePosition& tilePosition, const std::size_t id)
        {
            const auto it = mValuesByTilePosition.find(tilePosition);
            if (it != mValuesByTilePosition.end())
                it->second.erase(id);
        }
    };
}

#endif
