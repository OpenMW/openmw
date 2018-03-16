#include "trace.h"

#include "collisiontype.hpp"
#include "actor.hpp"
#include "convert.hpp"
#include "closestcollision.hpp"

namespace MWPhysics
{

void ActorTracer::doTrace(const btCollisionObject *actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    if (const auto collision = getClosestCollision(*actor, toBullet(start), toBullet(end), *world))
    {
        mFraction = collision->mFraction;
        mPlaneNormal = toOsg(collision->mNormal);
        mEndPos = toOsg(collision->mEnd);
        mHitPoint = toOsg(collision->mPoint);
        mHitObject = collision->mObject;
    }
    else
    {
        mEndPos = end;
        mPlaneNormal = osg::Vec3f(0.0f, 0.0f, 1.0f);
        mFraction = 1.0f;
        mHitPoint = end;
        mHitObject = NULL;
    }
}

void ActorTracer::findGround(const Actor* actor, const osg::Vec3f& start, const osg::Vec3f& end, const btCollisionWorld* world)
{
    if (const auto collision = getClosestCollision(*actor->getCollisionObject(), toBullet(start), toBullet(end), *world, CollisionType_Actor))
    {
        mFraction = collision->mFraction;
        mPlaneNormal = toOsg(collision->mNormal);
        mEndPos = toOsg(collision->mEnd);
    }
    else
    {
        mEndPos = end;
        mPlaneNormal = osg::Vec3f(0.0f, 0.0f, 1.0f);
        mFraction = 1.0f;
    }
}

}
