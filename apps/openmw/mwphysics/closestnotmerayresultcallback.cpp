#include "closestnotmerayresultcallback.hpp"

#include <algorithm>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/class.hpp"

#include "projectile.hpp"
#include "ptrholder.hpp"

namespace MWPhysics
{
    ClosestNotMeRayResultCallback::ClosestNotMeRayResultCallback(const btCollisionObject* me, const std::vector<const btCollisionObject*>& targets, const btVector3& from, const btVector3& to, int projId)
    : btCollisionWorld::ClosestRayResultCallback(from, to)
    , mMe(me), mTargets(targets), mProjectileId(projId)
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
                PtrHolder* holder = static_cast<PtrHolder*>(rayResult.m_collisionObject->getUserPointer());
                if (holder && !holder->getPtr().isEmpty() && holder->getPtr().getClass().isActor())
                    return 1.f;
            }
        }

        if (mProjectileId >= 0)
        {
            PtrHolder* holder = static_cast<PtrHolder*>(rayResult.m_collisionObject->getUserPointer());
            Projectile* projectile = dynamic_cast<Projectile*>(holder);
            if (projectile && projectile->getProjectileId() == mProjectileId)
                return 1.f;
        }

        return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
}
