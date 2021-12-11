#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHDBUTILS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHDBUTILS_H

#include "navmeshdb.hpp"

namespace DetourNavigator
{
    struct MeshSource;

    ShapeId resolveMeshSource(NavMeshDb& db, const MeshSource& source, ShapeId& nextShapeId);
}

#endif
