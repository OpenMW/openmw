#include "closestnotmeconvexresultcallback.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <components/misc/convert.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "collisiontype.hpp"
#include "projectile.hpp"

namespace MWPhysics
{
    ClosestNotMeConvexResultCallback::ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &motion, btScalar minCollisionDot)
    : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
      mMe(me), mMotion(motion), mMinCollisionDot(minCollisionDot)
    {
    }

    btScalar ClosestNotMeConvexResultCallback::addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
    {
        if (convexResult.m_hitCollisionObject == mMe)
            return btScalar(1);

        if (convexResult.m_hitCollisionObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Projectile)
        {
            Projectile* projectileHolder = static_cast<Projectile*>(convexResult.m_hitCollisionObject->getUserPointer());
            if (!projectileHolder->isActive())
                return btScalar(1);
            PtrHolder* targetHolder = static_cast<PtrHolder*>(mMe->getUserPointer());
            const MWWorld::Ptr target = targetHolder->getPtr();

            osg::Vec3f pos = Misc::Convert::makeOsgVec3f(convexResult.m_hitPointLocal);
            projectileHolder->hit(target, pos);
            return btScalar(1);
        }

        btVector3 hitNormalWorld;
        if (normalInWorldSpace)
            hitNormalWorld = convexResult.m_hitNormalLocal;
        else
        {
            ///need to transform normal into worldspace
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }

        // dot product of the motion vector against the collision contact normal
        btScalar dotCollision = mMotion.dot(hitNormalWorld);
        if (dotCollision <= mMinCollisionDot)
            return btScalar(1);

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }
}
