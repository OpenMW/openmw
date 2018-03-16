#include "closestcollision.hpp"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace MWPhysics
{
    static btVector3 getEnd(const btVector3& source, const btVector3& path, btScalar fraction)
    {
        return source + path * fraction;
    }

    ClosestNotMeConvexResultCallback::ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &up, btScalar minSlopeDot)
      : btCollisionWorld::ClosestConvexResultCallback(btVector3(0, 0, 0), btVector3(0, 0, 0)),
        mMe(me), mUp(up), mMinSlopeDot(minSlopeDot)
    {
    }

    btScalar ClosestNotMeConvexResultCallback::addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
    {
        if (convexResult.m_hitCollisionObject == mMe)
        {
            return btScalar(1);
        }

        btVector3 hitNormalWorld;
        if (normalInWorldSpace)
        {
            hitNormalWorld = convexResult.m_hitNormalLocal;
        }
        else
        {
            // need to transform normal into worldspace
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
        }

        const btScalar dotUp = mUp.dot(hitNormalWorld);
        if (dotUp < mMinSlopeDot)
        {
            return btScalar(1);
        }

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

    CollisionWithObjectCallback::CollisionWithObjectCallback(const btCollisionObject& object)
      : btCollisionWorld::ClosestConvexResultCallback(btVector3(0, 0, 0), btVector3(0, 0, 0))
      , mObject(&object)
    {
    }

    btScalar CollisionWithObjectCallback::addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
    {
        if (convexResult.m_hitCollisionObject != mObject)
        {
            return btScalar(1);
        }

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

    boost::optional<Collision> getClosestCollision(const btCollisionObject& actorObject,
            const btVector3& source, const btVector3& destination, const btCollisionWorld& collisionWorld,
            int excludeFilterMask)
    {
        const btTransform& actorTransform = actorObject.getWorldTransform();
        const btTransform from(actorTransform.getBasis(), source);
        const btTransform to(actorTransform.getBasis(), destination);

        ClosestNotMeConvexResultCallback callback(&actorObject, source - destination, btScalar(0));
        // Inherit the actor's collision group and mask
        callback.m_collisionFilterGroup = actorObject.getBroadphaseHandle()->m_collisionFilterGroup;
        callback.m_collisionFilterMask = actorObject.getBroadphaseHandle()->m_collisionFilterMask & ~excludeFilterMask;

        const btCollisionShape* shape = actorObject.getCollisionShape();
        assert(shape->isConvex());
        collisionWorld.convexSweepTest(static_cast<const btConvexShape*>(shape), from, to, callback);

        if (callback.hasHit())
        {
            return Collision
            {
                callback.m_hitCollisionObject,
                callback.m_closestHitFraction,
                callback.m_hitNormalWorld,
                callback.m_hitPointWorld,
                getEnd(source, destination - source, callback.m_closestHitFraction),
            };
        }

        return boost::none;
    }

    static const btScalar stepSizeUp = 2;

    static btCapsuleShapeZ makeReducedByHeightShape(const btCapsuleShapeZ& shape)
    {
        btCapsuleShapeZ result(shape.getRadius(), std::max(btScalar(0), btScalar(2) * shape.getHalfHeight() - stepSizeUp));
        result.setMargin(shape.getMargin());
        return result;
    }

    static btBoxShape makeReducedByHeightShape(const btBoxShape& shape)
    {
        const auto halfExtents = shape.getHalfExtentsWithoutMargin();
        btBoxShape result(btVector3(halfExtents.x(), halfExtents.y(), halfExtents.z() - stepSizeUp * btScalar(0.5)));
        result.setMargin(shape.getMargin());
        return result;
    }

    boost::optional<Collision> getClosestCollisionWithStepUp(const btCollisionObject& actorObject,
            const btVector3& source, const btVector3& destination, const btCollisionWorld& collisionWorld)
    {
        const auto shape = actorObject.getCollisionShape();

        if (!shape->isConvex())
        {
            throw std::invalid_argument("Actor shape is not convex");
        }

        const auto path = destination - source;

        ClosestNotMeConvexResultCallback reducedShapeCallback(&actorObject, -path, btScalar(0));
        reducedShapeCallback.m_collisionFilterGroup = actorObject.getBroadphaseHandle()->m_collisionFilterGroup;
        reducedShapeCallback.m_collisionFilterMask = actorObject.getBroadphaseHandle()->m_collisionFilterMask;

        const auto direction = path.normalized();
        const auto angle = btVector3(1, 0, 0).dot(direction);
        const btMatrix3x3 rotation(btQuaternion(btVector3(0, 0, 1), angle));

        if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE && static_cast<const btCapsuleShape*>(shape)->getUpAxis() == 2)
        {
            const auto casted = static_cast<const btCapsuleShapeZ*>(shape);
            const auto reducedShape = makeReducedByHeightShape(*casted);
            const auto z = casted->getHalfHeight() - reducedShape.getHalfHeight();
            const auto shift = rotation * btVector3(0, 0, z);
            const btTransform from(rotation, source + shift);
            const btTransform to(rotation, destination + shift);

            collisionWorld.convexSweepTest(&reducedShape, from, to, reducedShapeCallback);
        }
        else if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
        {
            const auto casted = static_cast<const btBoxShape*>(shape);
            const auto reducedShape = makeReducedByHeightShape(*casted);
            const auto z = casted->getHalfExtentsWithoutMargin().z() - reducedShape.getHalfExtentsWithoutMargin().z();
            const auto shift = rotation * btVector3(0, 0, z);
            const btTransform from(rotation, source + shift);
            const btTransform to(rotation, destination + shift);

            collisionWorld.convexSweepTest(&reducedShape, from, to, reducedShapeCallback);
        }
        else
        {
            throw std::logic_error(std::string("Shape reducing is not implemented for ") + shape->getName());
        }

        if (reducedShapeCallback.hasHit())
        {
            const btTransform from(rotation, source);
            const btTransform to(rotation, destination);

            CollisionWithObjectCallback fullShapeCallback(*reducedShapeCallback.m_hitCollisionObject);
            fullShapeCallback.m_collisionFilterGroup = actorObject.getBroadphaseHandle()->m_collisionFilterGroup;
            fullShapeCallback.m_collisionFilterMask = actorObject.getBroadphaseHandle()->m_collisionFilterMask;

            collisionWorld.convexSweepTest(static_cast<const btConvexShape*>(shape), from, to, fullShapeCallback);

            if (fullShapeCallback.hasHit())
            {
                return Collision
                {
                    fullShapeCallback.m_hitCollisionObject,
                    fullShapeCallback.m_closestHitFraction,
                    fullShapeCallback.m_hitNormalWorld,
                    fullShapeCallback.m_hitPointWorld,
                    getEnd(source, path, fullShapeCallback.m_closestHitFraction),
                };
            }

            return Collision
            {
                reducedShapeCallback.m_hitCollisionObject,
                reducedShapeCallback.m_closestHitFraction,
                reducedShapeCallback.m_hitNormalWorld,
                reducedShapeCallback.m_hitPointWorld,
                getEnd(source, path, reducedShapeCallback.m_closestHitFraction),
            };
        }

        return boost::none;
    }
}
