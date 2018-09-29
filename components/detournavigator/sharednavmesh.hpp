#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SHAREDNAVMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SHAREDNAVMESH_H

#include <components/misc/guarded.hpp>

#include <mutex>
#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    using NavMeshPtr = std::shared_ptr<dtNavMesh>;
    using SharedNavMesh = Misc::SharedGuarded<dtNavMesh>;
}

#endif
