#include "closestnotmerayresultcallback.hpp"

#include <algorithm>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "collisiontype.hpp"

namespace MWPhysics
{
    btScalar ClosestNotMeRayResultCallback::addSingleResult(
        btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
    {
        const auto* hitObject = rayResult.m_collisionObject;
        if (std::find(mIgnoreList.begin(), mIgnoreList.end(), hitObject) != mIgnoreList.end())
            return 1.f;

        if (hitObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor && !mTargets.empty())
        {
            if ((std::find(mTargets.begin(), mTargets.end(), hitObject) == mTargets.end()))
                return 1.f;
        }

        return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
}
