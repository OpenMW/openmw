#include "closestcollision.hpp"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

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
}
