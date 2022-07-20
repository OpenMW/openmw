#include "closestnotmerayresultcallback.hpp"

#include <algorithm>
#include <utility>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"
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
        const auto* hitObject = rayResult.m_collisionObject;
        if (hitObject == mMe)
            return 1.f;

        if (hitObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor && !mTargets.empty())
        {
            if ((std::find(mTargets.begin(), mTargets.end(), hitObject) == mTargets.end()))
                return 1.f;
        }

        return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
}
