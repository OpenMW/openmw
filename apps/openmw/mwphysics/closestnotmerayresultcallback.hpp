#ifndef OPENMW_MWPHYSICS_CLOSESTNOTMERAYRESULTCALLBACK_H
#define OPENMW_MWPHYSICS_CLOSESTNOTMERAYRESULTCALLBACK_H

#include <vector>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    class Projectile;

    class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
    {
    public:
        ClosestNotMeRayResultCallback(const btCollisionObject* me, std::vector<const btCollisionObject*> targets, const btVector3& from, const btVector3& to);

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override;

    private:
        const btCollisionObject* mMe;
        const std::vector<const btCollisionObject*> mTargets;
    };
}

#endif
