#ifndef OPENMW_MWPHYSICS_CLOSESTNOTMERAYRESULTCALLBACK_H
#define OPENMW_MWPHYSICS_CLOSESTNOTMERAYRESULTCALLBACK_H

#include <span>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    class Projectile;

    class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
    {
    public:
        explicit ClosestNotMeRayResultCallback(std::span<const btCollisionObject*> ignore,
            std::span<const btCollisionObject*> targets, const btVector3& from, const btVector3& to)
            : btCollisionWorld::ClosestRayResultCallback(from, to)
            , mIgnoreList(ignore)
            , mTargets(targets)
        {
        }

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override;

    private:
        const std::span<const btCollisionObject*> mIgnoreList;
        const std::span<const btCollisionObject*> mTargets;
    };
}

#endif
