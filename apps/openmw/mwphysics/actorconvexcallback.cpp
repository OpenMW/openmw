#include "actorconvexcallback.hpp"
#include "collisiontype.hpp"
#include "contacttestwrapper.h"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <components/misc/convert.hpp>

#include "projectile.hpp"

namespace MWPhysics
{
    namespace
    {
        struct ActorOverlapTester : public btCollisionWorld::ContactResultCallback
        {
            bool mOverlapping = false;

            btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* /*colObj0Wrap*/,
                int /*partId0*/, int /*index0*/, const btCollisionObjectWrapper* /*colObj1Wrap*/, int /*partId1*/,
                int /*index1*/) override
            {
                if (cp.getDistance() <= 0.0f)
                    mOverlapping = true;
                return 1;
            }
        };
    }

    btScalar ActorConvexCallback::addSingleResult(
        btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
    {
        if (convexResult.m_hitCollisionObject == mMe)
            return 1;

        // override data for actor-actor collisions
        // vanilla Morrowind seems to make overlapping actors collide as though they are both cylinders with a diameter
        // of the distance between them For some reason this doesn't work as well as it should when using capsules, but
        // it still helps a lot.
        if (convexResult.m_hitCollisionObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor)
        {
            ActorOverlapTester isOverlapping;
            // FIXME: This is absolutely terrible and bullet should feel terrible for not making contactPairTest
            // const-correct.
            ContactTestWrapper::contactPairTest(const_cast<btCollisionWorld*>(mWorld),
                const_cast<btCollisionObject*>(mMe), const_cast<btCollisionObject*>(convexResult.m_hitCollisionObject),
                isOverlapping);

            if (isOverlapping.mOverlapping)
            {
                auto originA = Misc::Convert::toOsg(mMe->getWorldTransform().getOrigin());
                auto originB = Misc::Convert::toOsg(convexResult.m_hitCollisionObject->getWorldTransform().getOrigin());
                osg::Vec3f motion = Misc::Convert::toOsg(mMotion);
                osg::Vec3f normal = (originA - originB);
                normal.z() = 0;
                normal.normalize();
                // only collide if horizontally moving towards the hit actor (note: the motion vector appears to be
                // inverted)
                // FIXME: This kinda screws with standing on actors that walk up slopes for some reason. Makes you fall
                // through them. It happens in vanilla Morrowind too, but much less often. I tried hunting down why but
                // couldn't figure it out. Possibly a stair stepping or ground ejection bug.
                if (normal * motion > 0.0f)
                {
                    convexResult.m_hitFraction = 0.0f;
                    convexResult.m_hitNormalLocal = Misc::Convert::toBullet(normal);
                    return ClosestConvexResultCallback::addSingleResult(convexResult, true);
                }
                else
                {
                    return 1;
                }
            }
        }
        if (convexResult.m_hitCollisionObject->getBroadphaseHandle()->m_collisionFilterGroup
            == CollisionType_Projectile)
        {
            auto* projectileHolder = static_cast<Projectile*>(convexResult.m_hitCollisionObject->getUserPointer());
            if (!projectileHolder->isActive())
                return 1;
            if (projectileHolder->isValidTarget(mMe))
                projectileHolder->hit(mMe, convexResult.m_hitPointLocal, convexResult.m_hitNormalLocal);
            return 1;
        }

        btVector3 hitNormalWorld;
        if (normalInWorldSpace)
            hitNormalWorld = convexResult.m_hitNormalLocal;
        else
        {
            /// need to transform normal into worldspace
            hitNormalWorld
                = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
        }

        // dot product of the motion vector against the collision contact normal
        btScalar dotCollision = mMotion.dot(hitNormalWorld);
        if (dotCollision <= mMinCollisionDot)
            return 1;

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }
}
