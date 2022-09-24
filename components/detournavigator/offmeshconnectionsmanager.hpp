#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OFFMESHCONNECTIONSMANAGER_H

#include "objectid.hpp"
#include "offmeshconnection.hpp"
#include "settings.hpp"
#include "tileposition.hpp"

#include <components/misc/guarded.hpp>

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

namespace DetourNavigator
{
    class OffMeshConnectionsManager
    {
    public:
        explicit OffMeshConnectionsManager(const RecastSettings& settings);

        void add(const ObjectId id, const OffMeshConnection& value);

        std::set<TilePosition> remove(const ObjectId id);

        std::vector<OffMeshConnection> get(const TilePosition& tilePosition) const;

    private:
        struct Values
        {
            std::multimap<ObjectId, OffMeshConnection> mById;
            std::map<TilePosition, std::unordered_set<ObjectId>> mByTilePosition;
        };

        const RecastSettings& mSettings;
        Misc::ScopeGuarded<Values> mValues;
    };
}

#endif
