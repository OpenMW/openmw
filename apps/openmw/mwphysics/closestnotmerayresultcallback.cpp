#include "closestnotmerayresultcallback.hpp"

#include <algorithm>
#include <utility>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/class.hpp"

#include "actor.hpp"
#include "collisiontype.hpp"
#include "projectile.hpp"
#include "ptrholder.hpp"

namespace MWPhysics
{
    ClosestNotMeRayResultCallback::ClosestNotMeRayResultCallback(const btCollisionObject* me, std::vector<const btCollisionObject*>  targets, const btVector3& from, const btVector3& to, Projectile* proj)
    : btCollisionWorld::ClosestRayResultCallback(from, to)
    , mMe(me), mTargets(std::move(targets)), mProjectile(proj)
    {
    }

    btScalar ClosestNotMeRayResultCallback::addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
    {
        if (rayResult.m_collisionObject == mMe)
            return 1.f;
        if (!mTargets.empty())
        {
            if ((std::find(mTargets.begin(), mTargets.end(), rayResult.m_collisionObject) == mTargets.end()))
            {
                auto* holder = static_cast<PtrHolder*>(rayResult.m_collisionObject->getUserPointer());
                if (holder && !holder->getPtr().isEmpty() && holder->getPtr().getClass().isActor())
                    return 1.f;
            }
        }

        btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        if (mProjectile)
        {
            auto* holder = static_cast<PtrHolder*>(rayResult.m_collisionObject->getUserPointer());
            if (auto* target = dynamic_cast<Actor*>(holder))
            {
                mProjectile->hit(target->getPtr(), m_hitPointWorld, m_hitNormalWorld);
            }
            else if (auto* target = dynamic_cast<Projectile*>(holder))
            {
                target->hit(mProjectile->getPtr(), m_hitPointWorld, m_hitNormalWorld);
                mProjectile->hit(target->getPtr(), m_hitPointWorld, m_hitNormalWorld);
            }
        }

        return rayResult.m_hitFraction;
    }
}
