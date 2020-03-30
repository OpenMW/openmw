#include "trace.h"

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include "collisiontype.hpp"
#include "actor.hpp"

namespace MWPhysics
{

class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &motion, btScalar minCollisionDot)
      : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
        mMe(me), mMotion(motion), mMinCollisionDot(minCollisionDot)
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

        // dot product of the motion vector against the collision contact normal
        btScalar dotCollision = mMotion.dot(hitNormalWorld);
        if(dotCollision <= mMinCollisionDot)
            return btScalar(1);

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

protected:
    const btCollisionObject *mMe;
    const btVector3 mMotion;
    const btScalar mMinCollisionDot;
};


void ActorTracer::doTrace(const btCollisionObject *actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    const btVector3 btstart = Misc::Convert::toBullet(start);
    const btVector3 btend = Misc::Convert::toBullet(end);

    const btTransform &trans = actor->getWorldTransform();
    btTransform from(trans);
    btTransform to(trans);
    from.setOrigin(btstart);
    to.setOrigin(btend);

    const btVector3 motion = btstart-btend;
    ClosestNotMeConvexResultCallback newTraceCallback(actor, motion, btScalar(0.0));
    // Inherit the actor's collision group and mask
    newTraceCallback.m_collisionFilterGroup = actor->getBroadphaseHandle()->m_collisionFilterGroup;
    newTraceCallback.m_collisionFilterMask = actor->getBroadphaseHandle()->m_collisionFilterMask;

    const btCollisionShape *shape = actor->getCollisionShape();
    assert(shape->isConvex());
    world->convexSweepTest(static_cast<const btConvexShape*>(shape), from, to, newTraceCallback);

    // Copy the hit data over to our trace results struct:
    if(newTraceCallback.hasHit())
    {
        mFraction = newTraceCallback.m_closestHitFraction;
        mPlaneNormal = Misc::Convert::toOsg(newTraceCallback.m_hitNormalWorld);
        mEndPos = (end-start)*mFraction + start;
        mHitPoint = Misc::Convert::toOsg(newTraceCallback.m_hitPointWorld);
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
    const btVector3 btstart = Misc::Convert::toBullet(start);
    const btVector3 btend = Misc::Convert::toBullet(end);

    const btTransform &trans = actor->getCollisionObject()->getWorldTransform();
    btTransform from(trans.getBasis(), btstart);
    btTransform to(trans.getBasis(), btend);

    const btVector3 motion = btstart-btend;
    ClosestNotMeConvexResultCallback newTraceCallback(actor->getCollisionObject(), motion, btScalar(0.0));
    // Inherit the actor's collision group and mask
    newTraceCallback.m_collisionFilterGroup = actor->getCollisionObject()->getBroadphaseHandle()->m_collisionFilterGroup;
    newTraceCallback.m_collisionFilterMask = actor->getCollisionObject()->getBroadphaseHandle()->m_collisionFilterMask;
    newTraceCallback.m_collisionFilterMask &= ~CollisionType_Actor;

    world->convexSweepTest(actor->getConvexShape(), from, to, newTraceCallback);
    if(newTraceCallback.hasHit())
    {
        mFraction = newTraceCallback.m_closestHitFraction;
        mPlaneNormal = Misc::Convert::toOsg(newTraceCallback.m_hitNormalWorld);
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
