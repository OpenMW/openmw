#include "collisionshapetype.hpp"

#include <stdexcept>
#include <string>

namespace DetourNavigator
{
    CollisionShapeType toCollisionShapeType(int value)
    {
        switch (static_cast<CollisionShapeType>(value))
        {
            case CollisionShapeType::Aabb:
            case CollisionShapeType::RotatingBox:
            case CollisionShapeType::Cylinder:
                return static_cast<CollisionShapeType>(value);
        }
        std::string error("Invalid CollisionShapeType value: \"");
        error += std::to_string(value);
        error += '"';
        throw std::invalid_argument(error);
    }
}
