#include "trace.h"

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>

#include "actor.hpp"
#include "actorconvexcallback.hpp"
#include "collisiontype.hpp"

namespace MWPhysics
{

    ActorConvexCallback sweepHelper(const btCollisionObject* actor, const btVector3& from, const btVector3& to,
        const btCollisionWorld* world, bool actorFilter)
    {
        const btTransform& trans = actor->getWorldTransform();
        btTransform transFrom(trans);
        btTransform transTo(trans);
        transFrom.setOrigin(from);
        transTo.setOrigin(to);

        const btCollisionShape* shape = actor->getCollisionShape();
        assert(shape->isConvex());

        const btVector3 motion
            = from - to; // FIXME: this is backwards; means ActorConvexCallback is doing dot product tests backwards too
        ActorConvexCallback traceCallback(actor, motion, btScalar(0.0), world);
        // Inherit the actor's collision group and mask
        traceCallback.m_collisionFilterGroup = actor->getBroadphaseHandle()->m_collisionFilterGroup;
        traceCallback.m_collisionFilterMask = actor->getBroadphaseHandle()->m_collisionFilterMask;
        if (actorFilter)
            traceCallback.m_collisionFilterMask &= ~CollisionType_Actor;

        world->convexSweepTest(static_cast<const btConvexShape*>(shape), transFrom, transTo, traceCallback);
        return traceCallback;
    }

    void ActorTracer::doTrace(const btCollisionObject* actor, const osg::Vec3f& start, const osg::Vec3f& end,
        const btCollisionWorld* world, bool attempt_short_trace)
    {
        const btVector3 btstart = Misc::Convert::toBullet(start);
        btVector3 btend = Misc::Convert::toBullet(end);

        // Because Bullet's collision trace tests touch *all* geometry in its path, a lot of long collision tests
        // will unnecessarily test against complex meshes that are dozens of units away. This wouldn't normally be
        // a problem, but bullet isn't the fastest in the world when it comes to doing tests against triangle meshes.
        // Therefore, we try out a short trace first, then only fall back to the full length trace if needed.
        // This trace needs to be at least a couple units long, but there's no one particular ideal length.
        // The length of 2.1 chosen here is a "works well in practice after testing a few random lengths" value.
        // (Also, we only do this short test if the intended collision trace is long enough for it to make sense.)
        const float fallbackLength = 2.1f;
        bool doingShortTrace = false;
        // For some reason, typical scenes perform a little better if we increase the threshold length for the length
        // test. (Multiplying by 2 in 'square distance' units gives us about 1.4x the threshold length. In benchmarks
        // this was
        //  slightly better for the performance of normal scenes than 4.0, and just plain better than 1.0.)
        if (attempt_short_trace && (btend - btstart).length2() > fallbackLength * fallbackLength * 2.0)
        {
            btend = btstart + (btend - btstart).normalized() * fallbackLength;
            doingShortTrace = true;
        }

        const auto traceCallback = sweepHelper(actor, btstart, btend, world, false);

        // Copy the hit data over to our trace results struct:
        if (traceCallback.hasHit())
        {
            mFraction = traceCallback.m_closestHitFraction;
            // ensure fraction is correct (covers intended distance traveled instead of actual distance traveled)
            if (doingShortTrace && (end - start).length2() > 0.0)
                mFraction *= (btend - btstart).length() / (end - start).length();
            mPlaneNormal = Misc::Convert::toOsg(traceCallback.m_hitNormalWorld);
            mEndPos = (end - start) * mFraction + start;
            mHitPoint = Misc::Convert::toOsg(traceCallback.m_hitPointWorld);
            mHitObject = traceCallback.m_hitCollisionObject;
        }
        else
        {
            if (doingShortTrace)
            {
                btend = Misc::Convert::toBullet(end);
                const auto newTraceCallback = sweepHelper(actor, btstart, btend, world, false);

                if (newTraceCallback.hasHit())
                {
                    mFraction = newTraceCallback.m_closestHitFraction;
                    mPlaneNormal = Misc::Convert::toOsg(newTraceCallback.m_hitNormalWorld);
                    mEndPos = (end - start) * mFraction + start;
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

    void ActorTracer::findGround(
        const Actor* actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
    {
        const auto traceCallback = sweepHelper(
            actor->getCollisionObject(), Misc::Convert::toBullet(start), Misc::Convert::toBullet(end), world, true);
        if (traceCallback.hasHit())
        {
            mFraction = traceCallback.m_closestHitFraction;
            mPlaneNormal = Misc::Convert::toOsg(traceCallback.m_hitNormalWorld);
            mEndPos = (end - start) * mFraction + start;
        }
        else
        {
            mEndPos = end;
            mPlaneNormal = osg::Vec3f(0.0f, 0.0f, 1.0f);
            mFraction = 1.0f;
        }
    }

}
