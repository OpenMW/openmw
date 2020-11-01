#ifndef OPENMW_MWPHYSICS_CLOSESTNOTMECONVEXRESULTCALLBACK_H
#define OPENMW_MWPHYSICS_CLOSESTNOTMECONVEXRESULTCALLBACK_H

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &motion, btScalar minCollisionDot, const btCollisionWorld * world);

        btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace) override;

    protected:
        const btCollisionObject *mMe;
        const btVector3 mMotion;
        const btScalar mMinCollisionDot;
        const btCollisionWorld * mWorld;
    };
}

#endif
