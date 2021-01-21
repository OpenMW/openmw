#include "closestnotmerayresultcallback.hpp"

#include <algorithm>
#include <utility>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/class.hpp"

#include "ptrholder.hpp"

namespace MWPhysics
{
    ClosestNotMeRayResultCallback::ClosestNotMeRayResultCallback(const btCollisionObject* me, std::vector<const btCollisionObject*>  targets, const btVector3& from, const btVector3& to)
    : btCollisionWorld::ClosestRayResultCallback(from, to)
    , mMe(me), mTargets(std::move(targets))
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

        return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
}
