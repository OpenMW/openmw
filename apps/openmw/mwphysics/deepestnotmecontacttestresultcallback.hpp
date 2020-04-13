#ifndef OPENMW_MWPHYSICS_DEEPESTNOTMECONTACTTESTRESULTCALLBACK_H
#define OPENMW_MWPHYSICS_DEEPESTNOTMECONTACTTESTRESULTCALLBACK_H

#include <vector>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    class DeepestNotMeContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
        const btCollisionObject* mMe;
        const std::vector<const btCollisionObject*> mTargets;

        // Store the real origin, since the shape's origin is its center
        btVector3 mOrigin;

    public:
        const btCollisionObject *mObject{nullptr};
        btVector3 mContactPoint{0,0,0};
        btScalar mLeastDistSqr;

        DeepestNotMeContactTestResultCallback(const btCollisionObject* me, const std::vector<const btCollisionObject*>& targets, const btVector3 &origin);

        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1);
    };
}

#endif
