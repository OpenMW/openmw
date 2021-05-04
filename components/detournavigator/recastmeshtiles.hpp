#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHTILE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHTILE_H

#include "tileposition.hpp"

#include <map>
#include <memory>

namespace DetourNavigator
{
    class RecastMesh;

    using RecastMeshTiles = std::map<TilePosition, std::shared_ptr<RecastMesh>>;
}

#endif
