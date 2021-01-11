#include <mutex>

#include "contacttestwrapper.h"

namespace MWPhysics
{
    // Concurrent calls to contactPairTest (and by extension contactTest) are forbidden.
    static std::mutex contactMutex;
    void ContactTestWrapper::contactTest(btCollisionWorld* collisionWorld, btCollisionObject* colObj, btCollisionWorld::ContactResultCallback& resultCallback)
    {
        std::unique_lock lock(contactMutex);
        collisionWorld->contactTest(colObj, resultCallback);
    }

    void ContactTestWrapper::contactPairTest(btCollisionWorld* collisionWorld, btCollisionObject* colObjA, btCollisionObject* colObjB, btCollisionWorld::ContactResultCallback& resultCallback)
    {
        std::unique_lock lock(contactMutex);
        collisionWorld->contactPairTest(colObjA, colObjB, resultCallback);
    }

}
