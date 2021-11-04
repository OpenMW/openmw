#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_AREATYPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_AREATYPE_H

#include <Recast.h>

#include <ostream>

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

    inline std::ostream& operator<<(std::ostream& stream, AreaType value)
    {
        switch (value)
        {
            case AreaType_null: return stream << "null";
            case AreaType_water: return stream << "water";
            case AreaType_door: return stream << "door";
            case AreaType_pathgrid: return stream << "pathgrid";
            case AreaType_ground: return stream << "ground";
        }
        return stream << "unknown area type (" << static_cast<std::underlying_type_t<AreaType>>(value) << ")";
    }
}

#endif
