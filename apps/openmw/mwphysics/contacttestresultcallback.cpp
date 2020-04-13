#include "contacttestresultcallback.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "ptrholder.hpp"

namespace MWPhysics
{
    ContactTestResultCallback::ContactTestResultCallback(const btCollisionObject* testedAgainst)
        : mTestedAgainst(testedAgainst)
    {
    }

    btScalar ContactTestResultCallback::addSingleResult(btManifoldPoint& cp,
                                                        const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                                        const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
    {
        const btCollisionObject* collisionObject = col0Wrap->m_collisionObject;
        if (collisionObject == mTestedAgainst)
            collisionObject = col1Wrap->m_collisionObject;
        PtrHolder* holder = static_cast<PtrHolder*>(collisionObject->getUserPointer());
        if (holder)
            mResult.push_back(holder->getPtr());
        return 0.f;
    }

}
