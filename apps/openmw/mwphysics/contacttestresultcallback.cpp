#include "contacttestresultcallback.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "components/misc/convert.hpp"

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
            mResult.emplace_back(ContactPoint{holder->getPtr(), Misc::Convert::toOsg(cp.m_positionWorldOnB), Misc::Convert::toOsg(cp.m_normalWorldOnB)});
        return 0.f;
    }

}
