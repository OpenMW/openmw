#include "trace.h"

#include <map>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include "collisiontype.hpp"
#include "actor.hpp"
#include "convert.hpp"

namespace MWPhysics
{

class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &up, btScalar minSlopeDot)
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

        btScalar dotUp = mUp.dot(hitNormalWorld);
        if(dotUp < mMinSlopeDot)
            return btScalar(1);

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

protected:
    const btCollisionObject *mMe;
    const btVector3 mUp;
    const btScalar mMinSlopeDot;
};


void ActorTracer::doTrace(const btCollisionObject *actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    const btVector3 btstart = toBullet(start);
    const btVector3 btend = toBullet(end);

    const btTransform &trans = actor->getWorldTransform();
    btTransform from(trans);
    btTransform to(trans);
    from.setOrigin(btstart);
    to.setOrigin(btend);

    ClosestNotMeConvexResultCallback newTraceCallback(actor, btstart-btend, btScalar(0.0));
    // Inherit the actor's collision group and mask
    newTraceCallback.m_collisionFilterGroup = actor->getBroadphaseHandle()->m_collisionFilterGroup;
    newTraceCallback.m_collisionFilterMask = actor->getBroadphaseHandle()->m_collisionFilterMask;

    const btCollisionShape *shape = actor->getCollisionShape();
    assert(shape->isConvex());
    world->convexSweepTest(static_cast<const btConvexShape*>(shape),
                                               from, to, newTraceCallback);

    // Copy the hit data over to our trace results struct:
    if(newTraceCallback.hasHit())
    {
        const btVector3& tracehitnormal = newTraceCallback.m_hitNormalWorld;
        mFraction = newTraceCallback.m_closestHitFraction;
        mPlaneNormal = osg::Vec3f(tracehitnormal.x(), tracehitnormal.y(), tracehitnormal.z());
        mEndPos = (end-start)*mFraction + start;
        mHitPoint = toOsg(newTraceCallback.m_hitPointWorld);
        mHitObject = newTraceCallback.m_hitCollisionObject;
    }
    else
    {
        mEndPos = end;
        mPlaneNormal = osg::Vec3f(0.0f, 0.0f, 1.0f);
        mFraction = 1.0f;
        mHitPoint = end;
        mHitObject = nullptr;
    }
}

void ActorTracer::findGround(const Actor* actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    const btVector3 btstart(start.x(), start.y(), start.z());
    const btVector3 btend(end.x(), end.y(), end.z());

    const btTransform &trans = actor->getCollisionObject()->getWorldTransform();
    btTransform from(trans.getBasis(), btstart);
    btTransform to(trans.getBasis(), btend);

    ClosestNotMeConvexResultCallback newTraceCallback(actor->getCollisionObject(), btstart-btend, btScalar(0.0));
    // Inherit the actor's collision group and mask
    newTraceCallback.m_collisionFilterGroup = actor->getCollisionObject()->getBroadphaseHandle()->m_collisionFilterGroup;
    newTraceCallback.m_collisionFilterMask = actor->getCollisionObject()->getBroadphaseHandle()->m_collisionFilterMask;
    newTraceCallback.m_collisionFilterMask &= ~CollisionType_Actor;

    world->convexSweepTest(actor->getConvexShape(), from, to, newTraceCallback);
    if(newTraceCallback.hasHit())
    {
        const btVector3& tracehitnormal = newTraceCallback.m_hitNormalWorld;
        mFraction = newTraceCallback.m_closestHitFraction;
        mPlaneNormal = osg::Vec3f(tracehitnormal.x(), tracehitnormal.y(), tracehitnormal.z());
        mEndPos = (end-start)*mFraction + start;
    }
    else
    {
        mEndPos = end;
        mPlaneNormal = osg::Vec3f(0.0f, 0.0f, 1.0f);
        mFraction = 1.0f;
    }
}

}
