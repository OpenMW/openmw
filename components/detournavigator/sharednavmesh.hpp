#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SHAREDNAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SHAREDNAVMESH_H

#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    using NavMeshPtr = std::shared_ptr<dtNavMesh>;
}

#endif
