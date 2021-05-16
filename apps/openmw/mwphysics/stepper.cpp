#include "stepper.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include "collisiontype.hpp"
#include "constants.hpp"
#include "movementsolver.hpp"

namespace MWPhysics
{
    static bool canStepDown(const ActorTracer &stepper)
    {
        if (!stepper.mHitObject)
            return false;
        static const float sMaxSlopeCos = std::cos(osg::DegreesToRadians(sMaxSlope));
        if (stepper.mPlaneNormal.z() <= sMaxSlopeCos)
            return false;

        return stepper.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup != CollisionType_Actor;
    }

    Stepper::Stepper(const btCollisionWorld *colWorld, const btCollisionObject *colObj)
        : mColWorld(colWorld)
        , mColObj(colObj)
    {
    }

    bool Stepper::step(osg::Vec3f &position, osg::Vec3f &velocity, float &remainingTime, const bool & onGround, bool firstIteration)
    {
        if(velocity.x() == 0.0 && velocity.y() == 0.0)
            return false;

        // Stairstepping algorithms work by moving up to avoid the step, moving forwards, then moving back down onto the ground.
        // This algorithm has a couple of minor problems, but they don't cause problems for sane geometry, and just prevent stepping on insane geometry.

        mUpStepper.doTrace(mColObj, position, position+osg::Vec3f(0.0f,0.0f,sStepSizeUp), mColWorld);

        float upDistance = 0;
        if(!mUpStepper.mHitObject)
            upDistance = sStepSizeUp;
        else if(mUpStepper.mFraction*sStepSizeUp > sCollisionMargin)
            upDistance = mUpStepper.mFraction*sStepSizeUp - sCollisionMargin;
        else
        {
            return false;
        }

        auto toMove = velocity * remainingTime;

        osg::Vec3f tracerPos = position + osg::Vec3f(0.0f, 0.0f, upDistance);

        osg::Vec3f tracerDest;
        auto normalMove = toMove;
        auto moveDistance = normalMove.normalize();
        // attempt 1: normal movement
        // attempt 2: fixed distance movement, only happens on the first movement solver iteration/bounce each frame to avoid a glitch
        // attempt 3: further, less tall fixed distance movement, same as above
        // If you're making a full conversion you should purge the logic for attempts 2 and 3. Attempts 2 and 3 just try to work around problems with vanilla Morrowind assets.
        int attempt = 0;
        float downStepSize = 0;
        while(attempt < 3)
        {
            attempt++;

            if(attempt == 1)
                tracerDest = tracerPos + toMove;
            else if (!sDoExtraStairHacks) // early out if we have extra hacks disabled
            {
                return false;
            }
            else if(attempt == 2)
            {
                moveDistance = sMinStep;
                tracerDest = tracerPos + normalMove*sMinStep;
            }
            else if(attempt == 3)
            {
                if(upDistance > sStepSizeUp)
                {
                    upDistance = sStepSizeUp;
                    tracerPos = position + osg::Vec3f(0.0f, 0.0f, upDistance);
                }
                moveDistance = sMinStep2;
                tracerDest = tracerPos + normalMove*sMinStep2;
            }

            mTracer.doTrace(mColObj, tracerPos, tracerDest, mColWorld);
            if(mTracer.mHitObject)
            {
                // map against what we hit, minus the safety margin
                moveDistance *= mTracer.mFraction;
                if(moveDistance <= sCollisionMargin) // didn't move enough to accomplish anything
                {
                    return false;
                }

                moveDistance -= sCollisionMargin;
                tracerDest = tracerPos + normalMove*moveDistance;

                // safely eject from what we hit by the safety margin
                auto tempDest = tracerDest + mTracer.mPlaneNormal*sCollisionMargin*2;

                ActorTracer tempTracer;
                tempTracer.doTrace(mColObj, tracerDest, tempDest, mColWorld);

                if(tempTracer.mFraction > 0.5f) // distance to any object is greater than sCollisionMargin (we checked sCollisionMargin*2 distance)
                {
                    auto effectiveFraction = tempTracer.mFraction*2.0f - 1.0f;
                    tracerDest += mTracer.mPlaneNormal*sCollisionMargin*effectiveFraction;
                }
            }

            if(attempt > 2) // do not allow stepping down below original height for attempt 3
                downStepSize = upDistance;
            else
                downStepSize = moveDistance + upDistance + sStepSizeDown;
            mDownStepper.doTrace(mColObj, tracerDest, tracerDest + osg::Vec3f(0.0f, 0.0f, -downStepSize), mColWorld);

            // can't step down onto air, non-walkable-slopes, or actors
            // NOTE: using a capsule causes isWalkableSlope (used in canStepDown) to fail on certain geometry that were intended to be valid at the bottoms of stairs
            // (like the bottoms of the staircases in aldruhn's guild of mages)
            // The old code worked around this by trying to do mTracer again with a fixed distance of sMinStep (10.0) but it caused all sorts of other problems.
            // Switched back to cylinders to avoid that and similer problems.
            if(canStepDown(mDownStepper))
            {
                break;
            }
            else
            {
                // do not try attempt 3 if we just tried attempt 2 and the horizontal distance was rather large
                // (forces actor to get snug against the defective ledge for attempt 3 to be tried)
                if(attempt == 2 && moveDistance > upDistance-(mDownStepper.mFraction*downStepSize))
                {
                    return false;
                }
                // do next attempt if first iteration of movement solver and not out of attempts
                if(firstIteration && attempt < 3)
                {
                    continue;
                }

                return false;
            }
        }

        // note: can't downstep onto actors so no need to pick safety margin
        float downDistance = 0;
        if(mDownStepper.mFraction*downStepSize > sCollisionMargin)
            downDistance = mDownStepper.mFraction*downStepSize - sCollisionMargin;

        if(downDistance-sCollisionMargin-sGroundOffset > upDistance && !onGround)
            return false;

        auto newpos = tracerDest + osg::Vec3f(0.0f, 0.0f, -downDistance);

        if((position-newpos).length2() < sCollisionMargin*sCollisionMargin)
            return false;

        if(mTracer.mHitObject)
        {
            auto planeNormal = mTracer.mPlaneNormal;
            if (onGround && !isWalkableSlope(planeNormal) && planeNormal.z() != 0)
            {
                planeNormal.z() = 0;
                planeNormal.normalize();
            }
            velocity = reject(velocity, planeNormal);
        }
        velocity = reject(velocity, mDownStepper.mPlaneNormal);

        position = newpos;

        remainingTime *= (1.0f-mTracer.mFraction); // remaining time is proportional to remaining distance
        return true;
    }
}
