#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_AREATYPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_AREATYPE_H

#include <Recast.h>

namespace DetourNavigator
{
    enum AreaType : unsigned char
    {
        AreaType_null = RC_NULL_AREA,
        AreaType_water,
        AreaType_door,
        AreaType_pathgrid,
        AreaType_ground = RC_WALKABLE_AREA,
    };

    struct AreaCosts
    {
        float mWater = 1.0f;
        float mDoor = 2.0f;
        float mPathgrid = 1.0f;
        float mGround = 1.0f;
    };
}

#endif
