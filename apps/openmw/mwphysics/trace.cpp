#include "trace.h"

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include "collisiontype.hpp"
#include "actor.hpp"

namespace MWPhysics
{

class ActorOverlapTester : public btCollisionWorld::ContactResultCallback
{
public:
    bool overlapping = false;

    btScalar addSingleResult(btManifoldPoint& cp,
        const btCollisionObjectWrapper* colObj0Wrap,
        int partId0,
        int index0,
        const btCollisionObjectWrapper* colObj1Wrap,
        int partId1,
        int index1)
    {
        if(cp.getDistance() <= 0.0f)
            overlapping = true;
        return btScalar(1);
    }
};

class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &up, btScalar minSlopeDot, const btCollisionWorld * world)
      : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
        mMe(me), mUp(up), mMinSlopeDot(minSlopeDot), mWorld(world)
    {
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
    {
        if(convexResult.m_hitCollisionObject == mMe)
            return btScalar( 1 );

        // override data for actor-actor collisions
        // vanilla Morrowind seems to make overlapping actors collide as though they are both cylinders with a diameter of the distance between them
        // For some reason this doesn't work as well as it should when using capsules, but it still helps a lot.
        if(convexResult.m_hitCollisionObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor)
        {
            ActorOverlapTester isOverlapping;
            // FIXME: This is absolutely terrible and bullet should feel terrible for not making contactPairTest const-correct.
            const_cast<btCollisionWorld*>(mWorld)->contactPairTest(const_cast<btCollisionObject*>(mMe), const_cast<btCollisionObject*>(convexResult.m_hitCollisionObject), isOverlapping);
            if(isOverlapping.overlapping)
            {
                auto originA = toOsg(mMe->getWorldTransform().getOrigin());
                auto originB = toOsg(convexResult.m_hitCollisionObject->getWorldTransform().getOrigin());
                osg::Vec3f motion = toOsg(mUp);
                osg::Vec3f normal = (originA-originB);
                normal.z() = 0;
                normal.normalize();
                // only collide if horizontally moving towards the hit actor (note: the motion vector appears to be inverted)
                // FIXME: This kinda screws with standing on actors that walk up slopes for some reason. Makes you fall through them.
                // It happens in vanilla Morrowind too, but much less often.
                // I tried hunting down why but couldn't figure it out. Possibly a stair stepping or ground ejection bug.
                if(normal * motion > 0.0f)
                {
                    convexResult.m_hitFraction = 0.0f;
                    convexResult.m_hitNormalLocal = toBullet(normal);
                    return ClosestConvexResultCallback::addSingleResult(convexResult, true);
                }
                else
                {
                    return btScalar(1);
                }
            }
        }

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
    const btCollisionWorld * mWorld;
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

    const auto motion = btstart-btend;

    ClosestNotMeConvexResultCallback newTraceCallback(actor, motion, btScalar(0.0), world);
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
        mPlaneNormal = toOsg(newTraceCallback.m_hitNormalWorld);
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
    const btVector3 btstart(start.x(), start.y(), start.z());
    const btVector3 btend(end.x(), end.y(), end.z());

    const btTransform &trans = actor->getCollisionObject()->getWorldTransform();
    btTransform from(trans.getBasis(), btstart);
    btTransform to(trans.getBasis(), btend);

    ClosestNotMeConvexResultCallback newTraceCallback(actor->getCollisionObject(), btstart-btend, btScalar(0.0), world);
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
