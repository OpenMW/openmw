#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_COLLISIONSHAPETYPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_COLLISIONSHAPETYPE_H

#include <cstdint>

namespace DetourNavigator
{
    enum class CollisionShapeType : std::uint8_t
    {
        Aabb = 0,
        RotatingBox = 1,
        Cylinder = 2,
    };

    CollisionShapeType toCollisionShapeType(int value);
}

#endif
