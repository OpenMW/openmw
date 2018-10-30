#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_AREATYPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_AREATYPE_H

#include <Recast.h>

namespace DetourNavigator
{
    enum AreaType : unsigned char
    {
        AreaType_null = RC_NULL_AREA,
        AreaType_water,
        AreaType_ground = RC_WALKABLE_AREA,
    };
}

#endif
