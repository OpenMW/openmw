#ifndef OPENMW_MWPHYSICS_CONTACTTESTWRAPPER_H
#define OPENMW_MWPHYSICS_CONTACTTESTWRAPPER_H

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

namespace MWPhysics
{
    struct ContactTestWrapper
    {
        static void contactTest(btCollisionWorld* collisionWorld, btCollisionObject* colObj, btCollisionWorld::ContactResultCallback& resultCallback);
        static void contactPairTest(btCollisionWorld* collisionWorld, btCollisionObject* colObjA, btCollisionObject* colObjB, btCollisionWorld::ContactResultCallback& resultCallback);
    };
}
#endif
