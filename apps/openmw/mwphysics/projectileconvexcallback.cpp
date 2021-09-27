#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/class.hpp"

#include "actor.hpp"
#include "collisiontype.hpp"
#include "projectile.hpp"
#include "projectileconvexcallback.hpp"
#include "ptrholder.hpp"

namespace MWPhysics
{
    ProjectileConvexCallback::ProjectileConvexCallback(const btCollisionObject* caster, const btCollisionObject* me, const btVector3& from, const btVector3& to, Projectile* proj)
        : btCollisionWorld::ClosestConvexResultCallback(from, to)
        , mCaster(caster)
        , mMe(me)
        , mProjectile(proj)
    {
        assert(mProjectile);
    }

    btScalar ProjectileConvexCallback::addSingleResult(btCollisionWorld::LocalConvexResult& result, bool normalInWorldSpace)
    {
        const auto* hitObject = result.m_hitCollisionObject;
        // don't hit the caster
        if (hitObject == mCaster)
            return 1.f;

        // don't hit the projectile
        if (hitObject == mMe)
            return 1.f;

        btCollisionWorld::ClosestConvexResultCallback::addSingleResult(result, normalInWorldSpace);
        switch (hitObject->getBroadphaseHandle()->m_collisionFilterGroup)
        {
            case CollisionType_Actor:
                {
                    if (!mProjectile->isValidTarget(hitObject))
                        return 1.f;
                    break;
                }
            case CollisionType_Projectile:
                {
                    auto* target = static_cast<Projectile*>(hitObject->getUserPointer());
                    if (!mProjectile->isValidTarget(target->getCasterCollisionObject()))
                        return 1.f;
                    target->hit(mMe, m_hitPointWorld, m_hitNormalWorld);
                    break;
                }
            case CollisionType_Water:
                {
                    mProjectile->setHitWater();
                    break;
                }
        }
        mProjectile->hit(hitObject, m_hitPointWorld, m_hitNormalWorld);

        return result.m_hitFraction;
    }

}

