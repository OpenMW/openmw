#ifndef OPENMW_COMPONENTS_BULLETHELPERS_COLLISIONOBJECT_H
#define OPENMW_COMPONENTS_BULLETHELPERS_COLLISIONOBJECT_H

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btTransform.h>

#include <memory>

namespace BulletHelpers
{
    inline std::unique_ptr<btCollisionObject> makeCollisionObject(btCollisionShape* shape,
        const btVector3& position, const btQuaternion& rotation)
    {
        std::unique_ptr<btCollisionObject> result = std::make_unique<btCollisionObject>();
        result->setCollisionShape(shape);
        result->setWorldTransform(btTransform(rotation, position));
        return result;
    }
}

#endif
