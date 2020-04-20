#include "stepper.hpp"

#include <limits>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include "collisiontype.hpp"
#include "constants.hpp"

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
        , mHaveMoved(true)
    {
    }

    bool Stepper::step(osg::Vec3f &position, const osg::Vec3f &toMove, float &remainingTime)
    {
        /*
         * Slide up an incline or set of stairs.  Should be called only after a
         * collision detection otherwise unnecessary tracing will be performed.
         *
         * NOTE: with a small change this method can be used to step over an obstacle
         * of height sStepSize.
         *
         * If successful return 'true' and update 'position' to the new possible
         * location and adjust 'remainingTime'.
         *
         * If not successful return 'false'.  May fail for these reasons:
         *    - can't move directly up from current position
         *    - having moved up by between epsilon() and sStepSize, can't move forward
         *    - having moved forward by between epsilon() and toMove,
         *        = moved down between 0 and just under sStepSize but slope was too steep, or
         *        = moved the full sStepSize down (FIXME: this could be a bug)
         *
         * Starting position.  Obstacle or stairs with height upto sStepSize in front.
         *
         *     +--+                          +--+       |XX
         *     |  | -------> toMove          |  |    +--+XX
         *     |  |                          |  |    |XXXXX
         *     |  | +--+                     |  | +--+XXXXX
         *     |  | |XX|                     |  | |XXXXXXXX
         *     +--+ +--+                     +--+ +--------
         *    ==============================================
         */

        /*
         * Try moving up sStepSize using stepper.
         * FIXME: does not work in case there is no front obstacle but there is one above
         *
         *     +--+                         +--+
         *     |  |                         |  |
         *     |  |                         |  |       |XX
         *     |  |                         |  |    +--+XX
         *     |  |                         |  |    |XXXXX
         *     +--+ +--+                    +--+ +--+XXXXX
         *          |XX|                         |XXXXXXXX
         *          +--+                         +--------
         *    ==============================================
         */
        if (mHaveMoved)
        {
            mHaveMoved = false;

            mUpStepper.doTrace(mColObj, position, position+osg::Vec3f(0.0f,0.0f,sStepSizeUp), mColWorld);
            if (mUpStepper.mFraction < std::numeric_limits<float>::epsilon())
                return false; // didn't even move the smallest representable amount
                              // (TODO: shouldn't this be larger? Why bother with such a small amount?)
        }

        /*
         * Try moving from the elevated position using tracer.
         *
         *                          +--+  +--+
         *                          |  |  |YY|   FIXME: collision with object YY
         *                          |  |  +--+
         *                          |  |
         *     <------------------->|  |
         *          +--+            +--+
         *          |XX|      the moved amount is toMove*tracer.mFraction
         *          +--+
         *    ==============================================
         */
        osg::Vec3f tracerPos = mUpStepper.mEndPos;
        mTracer.doTrace(mColObj, tracerPos, tracerPos + toMove, mColWorld);
        if (mTracer.mFraction < std::numeric_limits<float>::epsilon())
            return false; // didn't even move the smallest representable amount

        /*
         * Try moving back down sStepSizeDown using stepper.
         * NOTE: if there is an obstacle below (e.g. stairs), we'll be "stepping up".
         * Below diagram is the case where we "stepped over" an obstacle in front.
         *
         *                                +--+
         *                                |YY|
         *                          +--+  +--+
         *                          |  |
         *                          |  |
         *          +--+            |  |
         *          |XX|            |  |
         *          +--+            +--+
         *    ==============================================
         */
        mDownStepper.doTrace(mColObj, mTracer.mEndPos, mTracer.mEndPos-osg::Vec3f(0.0f,0.0f,sStepSizeDown), mColWorld);
        if (!canStepDown(mDownStepper))
        {
            // Try again with increased step length
            if (mTracer.mFraction < 1.0f || toMove.length2() > sMinStep*sMinStep)
                return false;

            osg::Vec3f direction = toMove;
            direction.normalize();
            mTracer.doTrace(mColObj, tracerPos, tracerPos + direction*sMinStep, mColWorld);
            if (mTracer.mFraction < 0.001f)
                return false;

            mDownStepper.doTrace(mColObj, mTracer.mEndPos, mTracer.mEndPos-osg::Vec3f(0.0f,0.0f,sStepSizeDown), mColWorld);
            if (!canStepDown(mDownStepper))
                return false;
        }

        if (mDownStepper.mFraction < 1.0f)
        {
            // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
            // TODO: stepper.mPlaneNormal does not appear to be reliable - needs more testing
            // NOTE: caller's variables 'position' & 'remainingTime' are modified here
            position = mDownStepper.mEndPos;
            remainingTime *= (1.0f-mTracer.mFraction); // remaining time is proportional to remaining distance
            mHaveMoved = true;
            return true;
        }
        return false;
    }
}
