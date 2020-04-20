#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHTILE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHTILE_H

#include "tileposition.hpp"
#include "recastmesh.hpp"

#include <map>
#include <memory>

namespace DetourNavigator
{
    using RecastMeshTiles = std::map<TilePosition, std::shared_ptr<RecastMesh>>;
}

#endif
