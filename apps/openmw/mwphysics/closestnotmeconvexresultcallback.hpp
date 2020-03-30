#ifndef OPENMW_MWPHYSICS_CLOSESTNOTMECONVEXRESULTCALLBACK_H
#define OPENMW_MWPHYSICS_CLOSESTNOTMECONVEXRESULTCALLBACK_H

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &motion, btScalar minCollisionDot);

        virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace);

    protected:
        const btCollisionObject *mMe;
        const btVector3 mMotion;
        const btScalar mMinCollisionDot;
    };
}

#endif
