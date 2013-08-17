
#include "trace.h"

#include <map>

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "physic.hpp"


namespace OEngine
{
namespace Physic
{

class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    ClosestNotMeConvexResultCallback(btCollisionObject *me, const btVector3 &up, btScalar minSlopeDot)
      : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
        mMe(me), mUp(up), mMinSlopeDot(minSlopeDot)
    {
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
    {
        if(convexResult.m_hitCollisionObject == mMe)
            return btScalar( 1 );

        btVector3 hitNormalWorld;
        if(normalInWorldSpace)
            hitNormalWorld = convexResult.m_hitNormalLocal;
        else
        {
            ///need to transform normal into worldspace
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
        }

        // NOTE : m_hitNormalLocal is not always vertical on the ground with a capsule or a box...

        btScalar dotUp = mUp.dot(hitNormalWorld);
        if(dotUp < mMinSlopeDot)
            return btScalar(1);

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

protected:
    btCollisionObject *mMe;
    const btVector3 mUp;
    const btScalar mMinSlopeDot;
};


void ActorTracer::doTrace(btCollisionObject *actor, const Ogre::Vector3 &start, const Ogre::Vector3 &end, const PhysicEngine *enginePass)
{
    const btVector3 btstart(start.x, start.y, start.z);
    const btVector3 btend(end.x, end.y, end.z);

    const btTransform &trans = actor->getWorldTransform();
    btTransform from(trans);
    btTransform to(trans);
    from.setOrigin(btstart);
    to.setOrigin(btend);

    ClosestNotMeConvexResultCallback newTraceCallback(actor, btstart-btend, btScalar(0.0));
    newTraceCallback.m_collisionFilterMask = OEngine::Physic::CollisionType_World |
                                             OEngine::Physic::CollisionType_HeightMap |
                                             OEngine::Physic::CollisionType_Actor;

    btCollisionShape *shape = actor->getCollisionShape();
    assert(shape->isConvex());
    enginePass->dynamicsWorld->convexSweepTest(static_cast<btConvexShape*>(shape),
                                               from, to, newTraceCallback);

    // Copy the hit data over to our trace results struct:
    if(newTraceCallback.hasHit())
    {
        const btVector3& tracehitnormal = newTraceCallback.m_hitNormalWorld;
        mFraction = newTraceCallback.m_closestHitFraction;
        mPlaneNormal = Ogre::Vector3(tracehitnormal.x(), tracehitnormal.y(), tracehitnormal.z());
        mEndPos = (end-start)*mFraction + start;
    }
    else
    {
        mEndPos = end;
        mPlaneNormal = Ogre::Vector3(0.0f, 0.0f, 1.0f);
        mFraction = 1.0f;
    }
}

}
}
