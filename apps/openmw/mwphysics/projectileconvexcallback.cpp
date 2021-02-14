#include "../mwworld/class.hpp"

#include "actor.hpp"
#include "collisiontype.hpp"
#include "projectile.hpp"
#include "projectileconvexcallback.hpp"
#include "ptrholder.hpp"

namespace MWPhysics
{
    ProjectileConvexCallback::ProjectileConvexCallback(const btCollisionObject* me, const btVector3& from, const btVector3& to, Projectile* proj)
        : btCollisionWorld::ClosestConvexResultCallback(from, to)
        , mMe(me), mProjectile(proj)
    {
        assert(mProjectile);
    }

    btScalar ProjectileConvexCallback::addSingleResult(btCollisionWorld::LocalConvexResult& result, bool normalInWorldSpace)
    {
        // don't hit the caster
        if (result.m_hitCollisionObject == mMe)
            return 1.f;

        // don't hit the projectile
        if (result.m_hitCollisionObject == mProjectile->getCollisionObject())
            return 1.f;

        btCollisionWorld::ClosestConvexResultCallback::addSingleResult(result, normalInWorldSpace);
        switch (result.m_hitCollisionObject->getBroadphaseHandle()->m_collisionFilterGroup)
        {
            case CollisionType_Actor:
                {
                    auto* target = static_cast<Actor*>(result.m_hitCollisionObject->getUserPointer());
                    if (!mProjectile->isValidTarget(target->getPtr()))
                        return 1.f;
                    mProjectile->hit(target->getPtr(), result.m_hitPointLocal, result.m_hitNormalLocal);
                    break;
                }
            case CollisionType_Projectile:
                {
                    auto* target = static_cast<Projectile*>(result.m_hitCollisionObject->getUserPointer());
                    if (!mProjectile->isValidTarget(target->getCaster()))
                        return 1.f;
                    target->hit(mProjectile->getPtr(), m_hitPointWorld, m_hitNormalWorld);
                    mProjectile->hit(target->getPtr(), m_hitPointWorld, m_hitNormalWorld);
                    break;
                }
            case CollisionType_Water:
                {
                    mProjectile->setWaterHitPosition(m_hitPointWorld);
                    if (mProjectile->canTraverseWater())
                        return 1.f;
                    mProjectile->hit(MWWorld::Ptr(), m_hitPointWorld, m_hitNormalWorld);
                    break;
                }
            default:
                {
                    auto* target = static_cast<PtrHolder*>(result.m_hitCollisionObject->getUserPointer());
                    auto ptr = target ? target->getPtr() : MWWorld::Ptr();
                    mProjectile->hit(ptr, m_hitPointWorld, m_hitNormalWorld);
                    break;
                }
        }

        return result.m_hitFraction;
    }

}

