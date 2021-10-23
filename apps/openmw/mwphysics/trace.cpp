#include "trace.h"

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include "collisiontype.hpp"
#include "actor.hpp"
#include "actorconvexcallback.hpp"

namespace MWPhysics
{

ActorConvexCallback sweepHelper(const btCollisionObject *actor, const btVector3& from, const btVector3& to, const btCollisionWorld* world, bool actorFilter)
{
    const btTransform &trans = actor->getWorldTransform();
    btTransform transFrom(trans);
    btTransform transTo(trans);
    transFrom.setOrigin(from);
    transTo.setOrigin(to);

    const btCollisionShape *shape = actor->getCollisionShape();
    assert(shape->isConvex());

    const btVector3 motion = from - to; // FIXME: this is backwards; means ActorConvexCallback is doing dot product tests backwards too
    ActorConvexCallback newTraceCallback(actor, motion, btScalar(0.0), world);
    // Inherit the actor's collision group and mask
    newTraceCallback.m_collisionFilterGroup = actor->getBroadphaseHandle()->m_collisionFilterGroup;
    newTraceCallback.m_collisionFilterMask = actor->getBroadphaseHandle()->m_collisionFilterMask;
    if(actorFilter)
        newTraceCallback.m_collisionFilterMask &= ~CollisionType_Actor;

    world->convexSweepTest(static_cast<const btConvexShape*>(shape), transFrom, transTo, newTraceCallback);
    return newTraceCallback;
}

void ActorTracer::doTrace(const btCollisionObject *actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    const btVector3 btstart = Misc::Convert::toBullet(start);
    btVector3 btend = Misc::Convert::toBullet(end);

    bool do_fallback = false;
    if((btend-btstart).length2() > 5.0*5.0)
    {
        btend = btstart + (btend-btstart).normalized()*5.0;
        do_fallback = true;
    }

    auto newTraceCallback = sweepHelper(actor, btstart, btend, world, false);

    // Copy the hit data over to our trace results struct:
    if(newTraceCallback.hasHit())
    {
        mFraction = newTraceCallback.m_closestHitFraction;
        if((end-start).length2() > 0.0)
            mFraction *= (btend-btstart).length() / (end-start).length();
        mPlaneNormal = Misc::Convert::toOsg(newTraceCallback.m_hitNormalWorld);
        mEndPos = (end-start)*mFraction + start;
        mHitPoint = Misc::Convert::toOsg(newTraceCallback.m_hitPointWorld);
        mHitObject = newTraceCallback.m_hitCollisionObject;
    }
    else
    {
        if(do_fallback)
        {
            btend = Misc::Convert::toBullet(end);
            auto newTraceCallback = sweepHelper(actor, btstart, btend, world, false);

            if(newTraceCallback.hasHit())
            {
                mFraction = newTraceCallback.m_closestHitFraction;
                mPlaneNormal = Misc::Convert::toOsg(newTraceCallback.m_hitNormalWorld);
                mEndPos = (end-start)*mFraction + start;
                mHitPoint = Misc::Convert::toOsg(newTraceCallback.m_hitPointWorld);
                mHitObject = newTraceCallback.m_hitCollisionObject;
                return;
            }
        }
        // fallthrough
        mEndPos = end;
        mPlaneNormal = osg::Vec3f(0.0f, 0.0f, 1.0f);
        mFraction = 1.0f;
        mHitPoint = end;
        mHitObject = nullptr;
    }
}

void ActorTracer::findGround(const Actor* actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    auto newTraceCallback = sweepHelper(actor->getCollisionObject(), Misc::Convert::toBullet(start), Misc::Convert::toBullet(end), world, true);
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
