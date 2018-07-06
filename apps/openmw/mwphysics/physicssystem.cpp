#include "physicssystem.hpp"

#include <iostream>
#include <stdexcept>

#include <osg/Group>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>

#include <LinearMath/btQuickprof.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/bulletshapemanager.hpp>

#include <components/esm/loadgmst.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/unrefqueue.hpp>

#include <components/nifosg/particle.hpp> // FindRecIndexVisitor

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/bulletdebugdraw.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"
#include "actor.hpp"
#include "convert.hpp"
#include "trace.h"

namespace MWPhysics
{

    static const float sMaxSlope = 49.0f;
    static const float sStepSizeUp = 34.0f;
    static const float sStepSizeDown = 62.0f;
    static const float sMinStep = 10.0f; // hack to skip over tiny unwalkable slopes
    static const float sGroundOffset = 1.0f;
    static const float sSafetyMargin = 0.01f;
    static const float sActorSafetyMargin = 0.02f;

    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 8;

    static bool isActor(const btCollisionObject *obj)
    {
        assert(obj);
        return obj->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor;
    }

    template <class Vec3>
    static bool isWalkableSlope(const Vec3 &normal)
    {
        static const float sMaxSlopeCos = std::cos(osg::DegreesToRadians(sMaxSlope));
        return (normal.z() > sMaxSlopeCos);
    }

    static bool canStepDown(const ActorTracer &stepper)
    {
        return stepper.mHitObject && isWalkableSlope(stepper.mPlaneNormal) && !isActor(stepper.mHitObject);
    }


    // vector projection (assumes normalized v)
    static inline osg::Vec3f project(const osg::Vec3f& u, const osg::Vec3f &v)
    {
        return v * (u * v);
    }

    // vector rejection
    static inline osg::Vec3f reject(const osg::Vec3f& direction, const osg::Vec3f &planeNormal)
    {
        return direction - project(direction, planeNormal);
    }

    static inline float pickSafetyMargin(const btCollisionObject *obj)
    {
        if(obj && isActor(obj))
            return sActorSafetyMargin;
        else
            return sSafetyMargin;
    }


    class Stepper
    {
    private:
        const btCollisionWorld *mColWorld;
        const btCollisionObject *mColObj;

        ActorTracer mTracer, mUpStepper, mDownStepper;

    public:
        Stepper(const btCollisionWorld *colWorld, const btCollisionObject *colObj)
            : mColWorld(colWorld)
            , mColObj(colObj)
        {}

        bool step(osg::Vec3f &position, osg::Vec3f &velocity, const osg::Vec3f &toMove, float &remainingTime, const bool & onGround, bool firstIteration)
        {
            if(toMove.x() == 0.0 && toMove.y() == 0.0)
                return false;

            // Stairstepping algorithms work by moving up to avoid the step, moving forwards, then moving back down onto the ground.
            // This algorithm has a couple of minor problems, but they don't cause problems for sane geometry, and just prevent stepping on insane geometry.

            mUpStepper.doTrace(mColObj, position, position+osg::Vec3f(0.0f,0.0f,sStepSizeUp), mColWorld);

            float upMargin = pickSafetyMargin(mUpStepper.mHitObject);
            float upDistance = 0;
            if(!mUpStepper.mHitObject)
                upDistance = sStepSizeUp;
            else if(mUpStepper.mFraction*sStepSizeUp > upMargin)
                upDistance = mUpStepper.mFraction*sStepSizeUp - upMargin;
            else
            {
                //std::cerr << "Warning: breaking steps A"  << std::endl;
                return false;
            }

            osg::Vec3f tracerPos = position + osg::Vec3f(0.0f, 0.0f, upDistance);

            osg::Vec3f tracerDest;
            auto normalMove = toMove;
            auto moveDistance = normalMove.normalize();
            // attempt 0: normal movement
            // attempt 1: fixed distance movement, only happens on the first movement solver iteration/bounce each frame to avoid a glitch
            int attempt = 0;
            float downStepSize;
            while(attempt < 2)
            {
                if(attempt == 0)
                    tracerDest = tracerPos + toMove;
                else if (!firstIteration) // first attempt failed and not on first movement solver iteration, can't retry
                    return false;
                else if(attempt == 1)
                {
                    moveDistance = sMinStep;
                    tracerDest = tracerPos + normalMove*sMinStep;
                }
                attempt++;

                mTracer.doTrace(mColObj, tracerPos, tracerDest, mColWorld);
                float moveMargin = pickSafetyMargin(mTracer.mHitObject);
                if(mTracer.mHitObject)
                {
                    // map against what we hit, minus the safety margin
                    moveDistance *= mTracer.mFraction;
                    if(moveDistance <= moveMargin) // didn't move enough to accomplish anything
                    {
                        //std::cerr << "Warning: breaking steps B"  << std::endl;
                        return false;
                    }

                    moveDistance -= moveMargin;
                    tracerDest = tracerPos + normalMove*moveDistance;

                    // safely eject from what we hit by the safety margin
                    auto tempDest = tracerDest + mTracer.mPlaneNormal*moveMargin*2;

                    ActorTracer tempTracer;
                    tempTracer.doTrace(mColObj, tracerDest, tempDest, mColWorld);

                    if(tempTracer.mFraction > 0.5f) // distance to any object is greater than moveMargin (we checked moveMargin*2 distance)
                    {
                        auto effectiveFraction = tempTracer.mFraction*2.0f - 1.0f;
                        tracerDest += mTracer.mPlaneNormal*moveMargin*effectiveFraction;
                    }
                }

                downStepSize = moveDistance + upDistance + sStepSizeDown;
                mDownStepper.doTrace(mColObj, tracerDest, tracerDest + osg::Vec3f(0.0f, 0.0f, -downStepSize), mColWorld);

                // can't step down onto air, non-walkable-slopes, or actors
                // NOTE: using a capsule makes isWalkableSlope fail on certain heights of steps that should be completely valid
                // (like the bottoms of the staircases in aldruhn's guild of mages)
                // The old code worked around this by trying to do mTracer again with a fixed distance of sMinStep (10.0) but it caused all sorts of other problems.
                if(canStepDown(mDownStepper))
                    break;
                else
                {
                    if(firstIteration && attempt-1 == 0)
                        continue;

                    //std::cerr << "Warning: breaking steps C " << std::endl;
                    //std::cerr << normalMove.x() << std::endl;
                    //std::cerr << normalMove.y() << std::endl;
                    //std::cerr << normalMove.z() << std::endl;
                    //std::cerr << moveDistance << std::endl;
                    return false;
                }
            }

            // note: can't downstep onto actors so no need to pick safety margin
            float downDistance = 0;
            if(mDownStepper.mFraction*downStepSize > sSafetyMargin)
                downDistance = mDownStepper.mFraction*downStepSize - sSafetyMargin;

            if(downDistance-sSafetyMargin-sGroundOffset > upDistance && !onGround)
            {
                //std::cerr << "Warning: breaking steps D " << std::endl;
                return false;
            }

            velocity = reject(velocity, mDownStepper.mPlaneNormal);

            position = tracerDest + osg::Vec3f(0.0f, 0.0f, -downDistance);

            remainingTime *= (1.0f-mTracer.mFraction); // remaining time is proportional to remaining distance
            return true;
        }
    };

    class MovementSolver
    {
    public:
        static osg::Vec3f traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, Actor* actor, btCollisionWorld* collisionWorld, float maxHeight)
        {
            osg::Vec3f offset = actor->getCollisionObjectPosition() - ptr.getRefData().getPosition().asVec3();

            ActorTracer tracer;
            tracer.findGround(actor, position + offset, position + offset - osg::Vec3f(0,0,maxHeight), collisionWorld);
            if(tracer.mFraction >= 1.0f)
            {
                actor->setOnGround(false);
                return position;
            }
            else
            {
                actor->setOnGround(true);

                // Check if we actually found a valid spawn point (use an infinitely thin ray this time).
                // Required for some broken door destinations in Morrowind.esm, where the spawn point
                // intersects with other geometry if the actor's base is taken into account
                btVector3 from = toBullet(position);
                btVector3 to = from - btVector3(0,0,maxHeight);

                btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
                resultCallback1.m_collisionFilterGroup = 0xff;
                resultCallback1.m_collisionFilterMask = CollisionType_World|CollisionType_HeightMap;

                collisionWorld->rayTest(from, to, resultCallback1);

                if (resultCallback1.hasHit() &&
                        ( (toOsg(resultCallback1.m_hitPointWorld) - (tracer.mEndPos-offset)).length2() > 35*35
                        || !isWalkableSlope(tracer.mPlaneNormal)))
                {
                    actor->setOnSlope(!isWalkableSlope(resultCallback1.m_hitNormalWorld));
                    return toOsg(resultCallback1.m_hitPointWorld) + osg::Vec3f(0.f, 0.f, sGroundOffset);
                }
                else
                {
                    actor->setOnSlope(!isWalkableSlope(tracer.mPlaneNormal));
                }

                return tracer.mEndPos-offset + osg::Vec3f(0.f, 0.f, sGroundOffset);
            }
        }

        static osg::Vec3f move(osg::Vec3f position, const MWWorld::Ptr &ptr, Actor* physicActor, const osg::Vec3f &movement, float time,
                                  bool isFlying, float waterlevel, float slowFall, const btCollisionWorld* collisionWorld,
                               std::map<MWWorld::Ptr, MWWorld::Ptr>& standingCollisionTracker)
        {
            const ESM::Position& refpos = ptr.getRefData().getPosition();
            // Early-out for totally static creatures
            // (Not sure if gravity should still apply?)
            if (!ptr.getClass().isMobile(ptr))
                return position;

            // Reset per-frame data
            physicActor->setWalkingOnWater(false);
            // Anything to collide with?
            if(!physicActor->getCollisionMode())
            {
                return position +  (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) *
                                    osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))
                                    ) * movement * time;
            }

            const btCollisionObject *colobj = physicActor->getCollisionObject();
            osg::Vec3f halfExtents = physicActor->getHalfExtents();

            // NOTE: here we don't account for the collision box translation (i.e. physicActor->getPosition() - refpos.pos).
            // That means the collision shape used for moving this actor is in a different spot than the collision shape
            // other actors are using to collide against this actor.
            // While this is strictly speaking wrong, it's needed for MW compatibility.
            position.z() += halfExtents.z();

            static const float fSwimHeightScale = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("fSwimHeightScale")->getFloat();
            float swimlevel = waterlevel + halfExtents.z() - (physicActor->getRenderingHalfExtents().z() * 2 * fSwimHeightScale);

            ActorTracer tracer;

            osg::Vec3f inertia = physicActor->getInertialForce();
            osg::Vec3f velocity;

            if(position.z() < swimlevel || isFlying)
            {
                velocity = (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) *
                            osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * movement;
            }
            else
            {
                velocity = (osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * movement;

                if ((velocity.z() > 0.f && physicActor->getOnGround() && !physicActor->getOnSlope())
                 || (velocity.z() > 0.f && velocity.z() + inertia.z() <= -velocity.z() && physicActor->getOnSlope())) // note: this should be less tolerant now that it's harder to get stuck in acute seams
                    inertia = velocity;
                else if (!physicActor->getOnGround() || physicActor->getOnSlope())
                    velocity = velocity + inertia;
            }

            // dead actors underwater will float to the surface, if the CharacterController tells us to do so
            if (movement.z() > 0 && ptr.getClass().getCreatureStats(ptr).isDead() && position.z() < swimlevel)
                velocity = osg::Vec3f(0,0,1) * 25;

            ptr.getClass().getMovementSettings(ptr).mPosition[2] = 0;

            // Now that we have the effective movement vector, apply wind forces to it
            if (MWBase::Environment::get().getWorld()->isInStorm())
            {
                osg::Vec3f stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
                float angleDegrees = osg::RadiansToDegrees(std::acos(stormDirection * velocity / (stormDirection.length() * velocity.length())));
                static const float fStromWalkMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fStromWalkMult")->getFloat();
                velocity *= 1.f-(fStromWalkMult * (angleDegrees/180.f));
            }

            Stepper stepper(collisionWorld, colobj);
            osg::Vec3f origVelocity = velocity;
            osg::Vec3f newPosition = position;
            /*
             * A loop to find newPosition using tracer, if successful different from the starting position.
             * nextpos is the local variable used to find potential newPosition, using velocity and remainingTime
             * The initial velocity was set earlier (see above).
             */
            float remainingTime = time;
            // to ensure actor state gets updated when climbing a stairstep while jumping
            bool forceGroundTest = false;
            // to control the "tiny slope" stair stepping hack
            bool noSlidingYet = true;
            // to handle simple acute crevices
            int numTimesSlid = 0;
            int numTimesSlidFallback = 0;
            osg::Vec3f lastSlideNormal(0,0,1);
            osg::Vec3f lastSlideFallbackNormal(0,0,1);

            for(int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
            {
                osg::Vec3f nextpos = newPosition + velocity * remainingTime;

                // Don't swim up out of water if not flying
                if(!isFlying && nextpos.z() > swimlevel && newPosition.z() < swimlevel)
                {
                    velocity.z() = 0;
                    continue;
                }

                if((newPosition - nextpos).length2() < 0.0001)
                {
                    // there's virtually no motion; break
                    break;
                }
                else
                {
                    // get tracing information between the current and desired position
                    tracer.doTrace(colobj, newPosition, nextpos, collisionWorld);

                    // if there is no obstruction, move to the new position and break
                    if(!tracer.mHitObject)
                    {
                        newPosition = tracer.mEndPos;
                        break;
                    }
                }

                // We are touching something.
                if (tracer.mFraction < 1E-9f)
                {
                    // Try to separate by backing off slighly to unstuck the solver
                    osg::Vec3f backOff = (newPosition - tracer.mHitPoint) * 1E-2f;
                    newPosition += backOff;
                }

                // We hit something. Check if we can step up.
                float hitHeight = tracer.mHitPoint.z() - tracer.mEndPos.z() + halfExtents.z();
                osg::Vec3f oldPosition = newPosition;
                bool result = false;
                // We can only step up:
                // - things that aren't definitely tall (hitHeight can return any point of contact, but will always return a low point of contact for short objects)
                // - non-actors
                // - things that are flat walls or facing upwards (no downwards-facing walls or ceilings)
                // note that we want to attempt stepping with too-steep slopes/sloped walls because they might be the side of a short step
                if (hitHeight < sStepSizeUp && !isActor(tracer.mHitObject) && tracer.mPlaneNormal.z() >= 0.0f)
                {
                    // Try to step up onto it.
                    // NOTE: step() is a proper procedure, it performs the stepping motion on its own if successful
                    result = stepper.step(newPosition, velocity, velocity*remainingTime, remainingTime, physicActor->getOnGround(), noSlidingYet);
                }
                noSlidingYet = false;
                if (result)
                {
                    // Prevents aquatic creatures from stairstepping onto land
                    if (ptr.getClass().isPureWaterCreature(ptr) && newPosition.z() + halfExtents.z() > waterlevel)
                        newPosition = oldPosition;
                    else
                        forceGroundTest = true;
                }
                else
                {
                    auto normVelocity = velocity;
                    auto moveDistance = normVelocity.normalize();

                    // Stairstepping failed, need to advance to and slide across whatever we hit

                    float traceMargin = pickSafetyMargin(tracer.mHitObject);
                    // advance if distance greater than safety margin
                    if(moveDistance*tracer.mFraction > traceMargin)
                    {
                        // hack: if it is the case that we are on the ground and it's a steep unwalkable slope, stay even further away from it than normal
                        // this hides some of the movement solver's shortcomings
                        if(physicActor->getOnGround() && !physicActor->getOnSlope() && !isWalkableSlope(tracer.mPlaneNormal) && moveDistance*tracer.mFraction > 0.2f+traceMargin)
                            newPosition = tracer.mEndPos - normVelocity*(0.2f+traceMargin);
                        else
                            newPosition = tracer.mEndPos - normVelocity*traceMargin;
                    }
                    // reduce remaining time to bounce around by how much we moved (ignoring the safety margin)
                    remainingTime *= 1.0f-tracer.mFraction;
                    // slide across it
                    auto virtualNormal = tracer.mPlaneNormal;
                    // if we're on the ground and it's too steep to walk, pretend it's a wall
                    if(physicActor->getOnGround() && !physicActor->getOnSlope() && !isWalkableSlope(virtualNormal))
                    {
                        virtualNormal.z() = 0;
                        virtualNormal.normalize();
                    }
                    // okay, actually slide across it - if it's coherent with the direction we're hitting it (i.e. we're hitting it from the front)
                    osg::Vec3f newVelocity = (virtualNormal * velocity <= 0.0f) ? reject(velocity, virtualNormal) : velocity;
                    // eject from whatever we hit, along the normal of contact
                    // (this makes it so that numerical instability doesn't render the motion-directional safety margin moot when hugging walls)
                    auto testPosition = newPosition + virtualNormal*traceMargin*2;
                    ActorTracer tempTracer;
                    tempTracer.doTrace(colobj, newPosition, testPosition, collisionWorld);
                    if(tempTracer.mFraction > 0.5f) // distance to any object is greater than traceMargin (we checked traceMargin*2 distance)
                    {
                        auto effectiveFraction = tempTracer.mFraction*2.0f - 1.0f;
                        newPosition += virtualNormal*traceMargin*effectiveFraction;
                    }

                    // Do not allow sliding upward if we're walking or jumping on land.
                    if(newPosition.z() >= swimlevel && !isFlying)
                        newVelocity.z() = std::min(newVelocity.z(), std::max(velocity.z(), 0.0f));

                    // check for colliding with acute convex corners; handling of acute crevices
                    if ((numTimesSlid > 0 && lastSlideNormal * virtualNormal <= 0.001f) || (numTimesSlid > 1 && lastSlideFallbackNormal * virtualNormal <= 0.001f))
                    {
                        // if we've already done crevice detection this it's probably stuck
                        if(numTimesSlidFallback > 1)
                            break;

                        // if we've already slid we should pick the best last normal to use
                        osg::Vec3f bestNormal = lastSlideNormal;
                        float product_older = 1;
                        float product_newer = 1;
                        float product_cross = 1;
                        float product_best = 1;

                        product_older = lastSlideNormal * virtualNormal;
                        product_best = product_older;
                        // if we've done this before and the third-most-recent collision normal isn't too similar to our current collision normal we might need to check for a three-sided pit
                        if(numTimesSlid > 1 && lastSlideFallbackNormal * virtualNormal < 0.99f)
                        {
                            product_newer = lastSlideFallbackNormal * virtualNormal;
                            product_cross = lastSlideFallbackNormal * lastSlideNormal;
                            // check for all three being acute or right angled; if they are, it's definitely a three-sided pit, we should bail early
                            if(product_older <= 0.001f && product_newer <= 0.001f && product_cross <= 0.001f)
                                break;
                            // otherwise we don't care about product_cross
                            if (product_newer <= 0.001f && product_newer <= product_older)
                            {
                                bestNormal = lastSlideFallbackNormal;
                                product_best = product_newer;
                            }
                        }
                        // note: the algorithm above only works in very simple cases, for very complex acute pits the solver will run out of iterations instead
                        if(product_best <= 0.001f)
                        {
                            // otherwise constrain our direction to that of the acute seam
                            osg::Vec3 constraintVector = bestNormal ^ virtualNormal;
                            constraintVector.normalize();

                            if(constraintVector.length2() > 0) // only if it's not zero length
                            {
                                newVelocity = project(velocity, constraintVector);
                                numTimesSlidFallback += 1;
                            }
                        }
                    }

                    // Break if our velocity hardly changed (?)
                    //if ((newVelocity-velocity).length2() < 0.01)
                    //    break;

                    // Break if our velocity got fully deflected
                    if (physicActor->getOnGround() && !physicActor->getOnSlope() && (newVelocity * origVelocity) <= 0.0f)
                        break;

                    numTimesSlid += 1;
                    lastSlideFallbackNormal = lastSlideNormal;
                    lastSlideNormal = virtualNormal;

                    velocity = newVelocity;
                }
            }

            bool isOnGround = false;
            bool isOnSlope = false;
            if (forceGroundTest || (inertia.z() <= 0.0f && newPosition.z() >= swimlevel))
            {
                // find ground
                osg::Vec3f from = newPosition;
                auto groundDistance = physicActor->getOnGround() ? (sStepSizeDown + 2*sGroundOffset) : (2*sGroundOffset);
                osg::Vec3f to = newPosition - osg::Vec3f(0,0,groundDistance);
                tracer.doTrace(colobj, from, to, collisionWorld);
                if(tracer.mFraction < 1.0f && !isActor(tracer.mHitObject))
                {
                    const btCollisionObject* standingOn = tracer.mHitObject;
                    PtrHolder* ptrHolder = static_cast<PtrHolder*>(standingOn->getUserPointer());
                    if (ptrHolder)
                        standingCollisionTracker[ptr] = ptrHolder->getPtr();

                    if (standingOn->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Water)
                        physicActor->setWalkingOnWater(true);

                    isOnGround = true;
                    isOnSlope = !isWalkableSlope(tracer.mPlaneNormal);

                    // note: ground can't be an actor so no need to pick safety margin
                    if(tracer.mFraction*groundDistance > sSafetyMargin)
                    {
                        newPosition = tracer.mEndPos;
                        newPosition.z() += sSafetyMargin;
                    }

                    // safely eject from ground (only if it's walkable; if it's unwalkable this makes us glide up it for a bit before gravity kicks back in)
                    if (!isFlying && !isOnSlope)
                    {
                        from = tracer.mEndPos;
                        to = tracer.mEndPos + osg::Vec3f(0, 0, sGroundOffset);
                        tracer.doTrace(colobj, from, to, collisionWorld);
                        if(!tracer.mHitObject)
                            newPosition.z() = tracer.mEndPos.z();
                        else if(tracer.mFraction*sGroundOffset > sSafetyMargin)
                            newPosition.z() += tracer.mFraction*sGroundOffset - sSafetyMargin;
                    }
                }
                else
                {
                    // standing on actors is not allowed (see above).
                    // in addition to that, apply a sliding effect away from the center of the actor,
                    // so that we do not stay suspended in air indefinitely.
                    if (tracer.mFraction < 1.0f && tracer.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor)
                    {
                        if (osg::Vec3f(velocity.x(), velocity.y(), 0).length2() < 100.f*100.f)
                        {
                            btVector3 aabbMin, aabbMax;
                            tracer.mHitObject->getCollisionShape()->getAabb(tracer.mHitObject->getWorldTransform(), aabbMin, aabbMax);
                            btVector3 center = (aabbMin + aabbMax) / 2.f;
                            inertia = osg::Vec3f(position.x() - center.x(), position.y() - center.y(), 0);
                            inertia.normalize();
                            inertia *= 100;
                        }
                    }

                    isOnGround = false;
                }
            }

            if((isOnGround && !isOnSlope) || newPosition.z() < swimlevel || isFlying)
                physicActor->setInertialForce(osg::Vec3f(0.f, 0.f, 0.f));
            else
            {
                inertia.z() += time * -627.2f;
                if (inertia.z() < 0)
                    inertia.z() *= slowFall;
                if (slowFall < 1.f) {
                    inertia.x() *= slowFall;
                    inertia.y() *= slowFall;
                }
                physicActor->setInertialForce(inertia);
            }
            physicActor->setOnGround(isOnGround);
            physicActor->setOnSlope(isOnSlope);

            newPosition.z() -= halfExtents.z(); // remove what was added at the beginning
            return newPosition;
        }
    };


    // ---------------------------------------------------------------

    class HeightField
    {
    public:
        HeightField(const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject)
        {
            mShape = new btHeightfieldTerrainShape(
                sqrtVerts, sqrtVerts, heights, 1,
                minH, maxH, 2,
                PHY_FLOAT, false
            );
            mShape->setUseDiamondSubdivision(true);
            mShape->setLocalScaling(btVector3(triSize, triSize, 1));

            btTransform transform(btQuaternion::getIdentity(),
                                  btVector3((x+0.5f) * triSize * (sqrtVerts-1),
                                            (y+0.5f) * triSize * (sqrtVerts-1),
                                            (maxH+minH)*0.5f));

            mCollisionObject = new btCollisionObject;
            mCollisionObject->setCollisionShape(mShape);
            mCollisionObject->setWorldTransform(transform);

            mHoldObject = holdObject;
        }
        ~HeightField()
        {
            delete mCollisionObject;
            delete mShape;
        }
        btCollisionObject* getCollisionObject()
        {
            return mCollisionObject;
        }

    private:
        btHeightfieldTerrainShape* mShape;
        btCollisionObject* mCollisionObject;
        osg::ref_ptr<const osg::Object> mHoldObject;

        void operator=(const HeightField&);
        HeightField(const HeightField&);
    };

    // --------------------------------------------------------------

    class Object : public PtrHolder
    {
    public:
        Object(const MWWorld::Ptr& ptr, osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance)
            : mShapeInstance(shapeInstance)
            , mSolid(true)
        {
            mPtr = ptr;

            mCollisionObject.reset(new btCollisionObject);
            mCollisionObject->setCollisionShape(shapeInstance->getCollisionShape());

            mCollisionObject->setUserPointer(static_cast<PtrHolder*>(this));

            setScale(ptr.getCellRef().getScale());
            setRotation(toBullet(ptr.getRefData().getBaseNode()->getAttitude()));
            const float* pos = ptr.getRefData().getPosition().pos;
            setOrigin(btVector3(pos[0], pos[1], pos[2]));
        }

        const Resource::BulletShapeInstance* getShapeInstance() const
        {
            return mShapeInstance.get();
        }

        void setScale(float scale)
        {
            mShapeInstance->getCollisionShape()->setLocalScaling(btVector3(scale,scale,scale));
        }

        void setRotation(const btQuaternion& quat)
        {
            mCollisionObject->getWorldTransform().setRotation(quat);
        }

        void setOrigin(const btVector3& vec)
        {
            mCollisionObject->getWorldTransform().setOrigin(vec);
        }

        btCollisionObject* getCollisionObject()
        {
            return mCollisionObject.get();
        }

        const btCollisionObject* getCollisionObject() const
        {
            return mCollisionObject.get();
        }

        /// Return solid flag. Not used by the object itself, true by default.
        bool isSolid() const
        {
            return mSolid;
        }

        void setSolid(bool solid)
        {
            mSolid = solid;
        }

        bool isAnimated() const
        {
            return !mShapeInstance->mAnimatedShapes.empty();
        }

        void animateCollisionShapes(btCollisionWorld* collisionWorld)
        {
            if (mShapeInstance->mAnimatedShapes.empty())
                return;

            assert (mShapeInstance->getCollisionShape()->isCompound());

            btCompoundShape* compound = static_cast<btCompoundShape*>(mShapeInstance->getCollisionShape());

            for (std::map<int, int>::const_iterator it = mShapeInstance->mAnimatedShapes.begin(); it != mShapeInstance->mAnimatedShapes.end(); ++it)
            {
                int recIndex = it->first;
                int shapeIndex = it->second;

                std::map<int, osg::NodePath>::iterator nodePathFound = mRecIndexToNodePath.find(recIndex);
                if (nodePathFound == mRecIndexToNodePath.end())
                {
                    NifOsg::FindGroupByRecIndex visitor(recIndex);
                    mPtr.getRefData().getBaseNode()->accept(visitor);
                    if (!visitor.mFound)
                    {
                        std::cerr << "Error: animateCollisionShapes can't find node " << recIndex << " for " << mPtr.getCellRef().getRefId() << std::endl;
                        return;
                    }
                    osg::NodePath nodePath = visitor.mFoundPath;
                    nodePath.erase(nodePath.begin());
                    nodePathFound = mRecIndexToNodePath.insert(std::make_pair(recIndex, nodePath)).first;
                }

                osg::NodePath& nodePath = nodePathFound->second;
                osg::Matrixf matrix = osg::computeLocalToWorld(nodePath);
                osg::Vec3f scale = matrix.getScale();
                matrix.orthoNormalize(matrix);

                btTransform transform;
                transform.setOrigin(toBullet(matrix.getTrans()) * compound->getLocalScaling());
                for (int i=0; i<3; ++i)
                    for (int j=0; j<3; ++j)
                        transform.getBasis()[i][j] = matrix(j,i); // NB column/row major difference

                if (compound->getLocalScaling() * toBullet(scale) != compound->getChildShape(shapeIndex)->getLocalScaling())
                    compound->getChildShape(shapeIndex)->setLocalScaling(compound->getLocalScaling() * toBullet(scale));
                if (!(transform == compound->getChildTransform(shapeIndex)))
                    compound->updateChildTransform(shapeIndex, transform);
            }

            collisionWorld->updateSingleAabb(mCollisionObject.get());
        }

    private:
        std::unique_ptr<btCollisionObject> mCollisionObject;
        osg::ref_ptr<Resource::BulletShapeInstance> mShapeInstance;
        std::map<int, osg::NodePath> mRecIndexToNodePath;
        bool mSolid;
    };

    // ---------------------------------------------------------------

    PhysicsSystem::PhysicsSystem(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> parentNode)
        : mShapeManager(new Resource::BulletShapeManager(resourceSystem->getVFS(), resourceSystem->getSceneManager(), resourceSystem->getNifFileManager()))
        , mResourceSystem(resourceSystem)
        , mDebugDrawEnabled(false)
        , mTimeAccum(0.0f)
        , mWaterHeight(0)
        , mWaterEnabled(false)
        , mParentNode(parentNode)
        , mPhysicsDt(1.f / 60.f)
    {
        mResourceSystem->addResourceManager(mShapeManager.get());

        mCollisionConfiguration = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
        mBroadphase = new btDbvtBroadphase();

        mCollisionWorld = new btCollisionWorld(mDispatcher, mBroadphase, mCollisionConfiguration);

        // Don't update AABBs of all objects every frame. Most objects in MW are static, so we don't need this.
        // Should a "static" object ever be moved, we have to update its AABB manually using DynamicsWorld::updateSingleAabb.
        mCollisionWorld->setForceUpdateAllAabbs(false);

        // Check if a user decided to override a physics system FPS
        const char* env = getenv("OPENMW_PHYSICS_FPS");
        if (env)
        {
            float physFramerate = std::atof(env);
            if (physFramerate > 0)
            {
                mPhysicsDt = 1.f / physFramerate;
                std::cerr << "Warning: physics framerate was overridden (a new value is " << physFramerate << ")."  << std::endl;
            }
        }
    }

    PhysicsSystem::~PhysicsSystem()
    {
        mResourceSystem->removeResourceManager(mShapeManager.get());

        if (mWaterCollisionObject.get())
            mCollisionWorld->removeCollisionObject(mWaterCollisionObject.get());

        for (HeightFieldMap::iterator it = mHeightFields.begin(); it != mHeightFields.end(); ++it)
        {
            mCollisionWorld->removeCollisionObject(it->second->getCollisionObject());
            delete it->second;
        }

        for (ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); ++it)
        {
            mCollisionWorld->removeCollisionObject(it->second->getCollisionObject());
            delete it->second;
        }

        for (ActorMap::iterator it = mActors.begin(); it != mActors.end(); ++it)
        {
            delete it->second;
        }

        delete mCollisionWorld;
        delete mCollisionConfiguration;
        delete mDispatcher;
        delete mBroadphase;
    }

    void PhysicsSystem::setUnrefQueue(SceneUtil::UnrefQueue *unrefQueue)
    {
        mUnrefQueue = unrefQueue;
    }

    Resource::BulletShapeManager *PhysicsSystem::getShapeManager()
    {
        return mShapeManager.get();
    }

    bool PhysicsSystem::toggleDebugRendering()
    {
        mDebugDrawEnabled = !mDebugDrawEnabled;

        if (mDebugDrawEnabled && !mDebugDrawer.get())
        {
            mDebugDrawer.reset(new MWRender::DebugDrawer(mParentNode, mCollisionWorld));
            mCollisionWorld->setDebugDrawer(mDebugDrawer.get());
            mDebugDrawer->setDebugMode(mDebugDrawEnabled);
        }
        else if (mDebugDrawer.get())
            mDebugDrawer->setDebugMode(mDebugDrawEnabled);
        return mDebugDrawEnabled;
    }

    void PhysicsSystem::markAsNonSolid(const MWWorld::ConstPtr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found == mObjects.end())
            return;

        found->second->setSolid(false);
    }

    bool PhysicsSystem::isOnSolidGround (const MWWorld::Ptr& actor) const
    {
        const Actor* physactor = getActor(actor);
        if (!physactor || !physactor->getOnGround())
            return false;

        CollisionMap::const_iterator found = mStandingCollisions.find(actor);
        if (found == mStandingCollisions.end())
            return true; // assume standing on terrain (which is a non-object, so not collision tracked)

        ObjectMap::const_iterator foundObj = mObjects.find(found->second);
        if (foundObj == mObjects.end())
            return false;

        if (!foundObj->second->isSolid())
            return false;

        return true;
    }

    class DeepestNotMeContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
        const btCollisionObject* mMe;
        const std::vector<const btCollisionObject*> mTargets;

        // Store the real origin, since the shape's origin is its center
        btVector3 mOrigin;

    public:
        const btCollisionObject *mObject;
        btVector3 mContactPoint;
        btScalar mLeastDistSqr;

        DeepestNotMeContactTestResultCallback(const btCollisionObject* me, const std::vector<const btCollisionObject*>& targets, const btVector3 &origin)
          : mMe(me), mTargets(targets), mOrigin(origin), mObject(NULL), mContactPoint(0,0,0),
            mLeastDistSqr(std::numeric_limits<float>::max())
        { }

        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
        {
            const btCollisionObject* collisionObject = col1Wrap->m_collisionObject;
            if (collisionObject != mMe)
            {
                if (!mTargets.empty())
                {
                    if ((std::find(mTargets.begin(), mTargets.end(), collisionObject) == mTargets.end()))
                    {
                        PtrHolder* holder = static_cast<PtrHolder*>(collisionObject->getUserPointer());
                        if (holder && !holder->getPtr().isEmpty() && holder->getPtr().getClass().isActor())
                            return 0.f;
                    }
                }

                btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
                if(!mObject || distsqr < mLeastDistSqr)
                {
                    mObject = collisionObject;
                    mLeastDistSqr = distsqr;
                    mContactPoint = cp.getPositionWorldOnA();
                }
            }

            return 0.f;
        }
    };

    std::pair<MWWorld::Ptr, osg::Vec3f> PhysicsSystem::getHitContact(const MWWorld::ConstPtr& actor,
                                                                     const osg::Vec3f &origin,
                                                                     const osg::Quat &orient,
                                                                     float queryDistance, std::vector<MWWorld::Ptr> targets)
    {
        // First of all, try to hit where you aim to
        int hitmask = CollisionType_World | CollisionType_Door | CollisionType_HeightMap | CollisionType_Actor;
        RayResult result = castRay(origin, origin + (orient * osg::Vec3f(0.0f, queryDistance, 0.0f)), actor, targets, CollisionType_Actor, hitmask);

        if (result.mHit)
        {
            return std::make_pair(result.mHitObject, result.mHitPos);
        }

        // Use cone shape as fallback
        const MWWorld::Store<ESM::GameSetting> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        btConeShape shape (osg::DegreesToRadians(store.find("fCombatAngleXY")->getFloat()/2.0f), queryDistance);
        shape.setLocalScaling(btVector3(1, 1, osg::DegreesToRadians(store.find("fCombatAngleZ")->getFloat()/2.0f) /
                                              shape.getRadius()));

        // The shape origin is its center, so we have to move it forward by half the length. The
        // real origin will be provided to getFilteredContact to find the closest.
        osg::Vec3f center = origin + (orient * osg::Vec3f(0.0f, queryDistance*0.5f, 0.0f));

        btCollisionObject object;
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(toBullet(orient), toBullet(center)));

        const btCollisionObject* me = NULL;
        std::vector<const btCollisionObject*> targetCollisionObjects;

        const Actor* physactor = getActor(actor);
        if (physactor)
            me = physactor->getCollisionObject();

        if (!targets.empty())
        {
            for (std::vector<MWWorld::Ptr>::const_iterator it = targets.begin(); it != targets.end(); ++it)
            {
                const Actor* physactor2 = getActor(*it);
                if (physactor2)
                    targetCollisionObjects.push_back(physactor2->getCollisionObject());
            }
        }

        DeepestNotMeContactTestResultCallback resultCallback(me, targetCollisionObjects, toBullet(origin));
        resultCallback.m_collisionFilterGroup = CollisionType_Actor;
        resultCallback.m_collisionFilterMask = CollisionType_World | CollisionType_Door | CollisionType_HeightMap | CollisionType_Actor;
        mCollisionWorld->contactTest(&object, resultCallback);

        if (resultCallback.mObject)
        {
            PtrHolder* holder = static_cast<PtrHolder*>(resultCallback.mObject->getUserPointer());
            if (holder)
                return std::make_pair(holder->getPtr(), toOsg(resultCallback.mContactPoint));
        }
        return std::make_pair(MWWorld::Ptr(), osg::Vec3f());
    }

    float PhysicsSystem::getHitDistance(const osg::Vec3f &point, const MWWorld::ConstPtr &target) const
    {
        btCollisionObject* targetCollisionObj = NULL;
        const Actor* actor = getActor(target);
        if (actor)
            targetCollisionObj = actor->getCollisionObject();
        if (!targetCollisionObj)
            return 0.f;

        btTransform rayFrom;
        rayFrom.setIdentity();
        rayFrom.setOrigin(toBullet(point));

        // target the collision object's world origin, this should be the center of the collision object
        btTransform rayTo;
        rayTo.setIdentity();
        rayTo.setOrigin(targetCollisionObj->getWorldTransform().getOrigin());

        btCollisionWorld::ClosestRayResultCallback cb(rayFrom.getOrigin(), rayTo.getOrigin());

        btCollisionWorld::rayTestSingle(rayFrom, rayTo, targetCollisionObj, targetCollisionObj->getCollisionShape(), targetCollisionObj->getWorldTransform(), cb);
        if (!cb.hasHit())
        {
            // didn't hit the target. this could happen if point is already inside the collision box
            return 0.f;
        }
        else
            return (point - toOsg(cb.m_hitPointWorld)).length();
    }

    class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
    {
    public:
        ClosestNotMeRayResultCallback(const btCollisionObject* me, const std::vector<const btCollisionObject*>& targets, const btVector3& from, const btVector3& to)
            : btCollisionWorld::ClosestRayResultCallback(from, to)
            , mMe(me), mTargets(targets)
        {
        }

        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
        {
            if (rayResult.m_collisionObject == mMe)
                return 1.f;
            if (!mTargets.empty())
            {
                if ((std::find(mTargets.begin(), mTargets.end(), rayResult.m_collisionObject) == mTargets.end()))
                {
                    PtrHolder* holder = static_cast<PtrHolder*>(rayResult.m_collisionObject->getUserPointer());
                    if (holder && !holder->getPtr().isEmpty() && holder->getPtr().getClass().isActor())
                        return 1.f;
                }
            }
            return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    private:
        const btCollisionObject* mMe;
        const std::vector<const btCollisionObject*> mTargets;
    };

    PhysicsSystem::RayResult PhysicsSystem::castRay(const osg::Vec3f &from, const osg::Vec3f &to, const MWWorld::ConstPtr& ignore, std::vector<MWWorld::Ptr> targets, int mask, int group) const
    {
        btVector3 btFrom = toBullet(from);
        btVector3 btTo = toBullet(to);

        const btCollisionObject* me = NULL;
        std::vector<const btCollisionObject*> targetCollisionObjects;

        if (!ignore.isEmpty())
        {
            const Actor* actor = getActor(ignore);
            if (actor)
                me = actor->getCollisionObject();
            else
            {
                const Object* object = getObject(ignore);
                if (object)
                    me = object->getCollisionObject();
            }
        }

        if (!targets.empty())
        {
            for (std::vector<MWWorld::Ptr>::const_iterator it = targets.begin(); it != targets.end(); ++it)
            {
                const Actor* actor = getActor(*it);
                if (actor)
                    targetCollisionObjects.push_back(actor->getCollisionObject());
            }
        }

        ClosestNotMeRayResultCallback resultCallback(me, targetCollisionObjects, btFrom, btTo);
        resultCallback.m_collisionFilterGroup = group;
        resultCallback.m_collisionFilterMask = mask;

        mCollisionWorld->rayTest(btFrom, btTo, resultCallback);

        RayResult result;
        result.mHit = resultCallback.hasHit();
        if (resultCallback.hasHit())
        {
            result.mHitPos = toOsg(resultCallback.m_hitPointWorld);
            result.mHitNormal = toOsg(resultCallback.m_hitNormalWorld);
            if (PtrHolder* ptrHolder = static_cast<PtrHolder*>(resultCallback.m_collisionObject->getUserPointer()))
                result.mHitObject = ptrHolder->getPtr();
        }
        return result;
    }

    PhysicsSystem::RayResult PhysicsSystem::castSphere(const osg::Vec3f &from, const osg::Vec3f &to, float radius)
    {
        btCollisionWorld::ClosestConvexResultCallback callback(toBullet(from), toBullet(to));
        callback.m_collisionFilterGroup = 0xff;
        callback.m_collisionFilterMask = CollisionType_World|CollisionType_HeightMap|CollisionType_Door;

        btSphereShape shape(radius);
        const btQuaternion btrot = btQuaternion::getIdentity();

        btTransform from_ (btrot, toBullet(from));
        btTransform to_ (btrot, toBullet(to));

        mCollisionWorld->convexSweepTest(&shape, from_, to_, callback);

        RayResult result;
        result.mHit = callback.hasHit();
        if (result.mHit)
        {
            result.mHitPos = toOsg(callback.m_hitPointWorld);
            result.mHitNormal = toOsg(callback.m_hitNormalWorld);
        }
        return result;
    }

    bool PhysicsSystem::getLineOfSight(const MWWorld::ConstPtr &actor1, const MWWorld::ConstPtr &actor2) const
    {
        const Actor* physactor1 = getActor(actor1);
        const Actor* physactor2 = getActor(actor2);

        if (!physactor1 || !physactor2)
            return false;

        osg::Vec3f pos1 (physactor1->getCollisionObjectPosition() + osg::Vec3f(0,0,physactor1->getHalfExtents().z() * 0.9)); // eye level
        osg::Vec3f pos2 (physactor2->getCollisionObjectPosition() + osg::Vec3f(0,0,physactor2->getHalfExtents().z() * 0.9));

        RayResult result = castRay(pos1, pos2, MWWorld::ConstPtr(), std::vector<MWWorld::Ptr>(), CollisionType_World|CollisionType_HeightMap|CollisionType_Door);

        return !result.mHit;
    }

    bool PhysicsSystem::isOnGround(const MWWorld::Ptr &actor)
    {
        Actor* physactor = getActor(actor);
        return physactor && physactor->getOnGround();
    }

    bool PhysicsSystem::canMoveToWaterSurface(const MWWorld::ConstPtr &actor, const float waterlevel)
    {
        const Actor* physicActor = getActor(actor);
        if (!physicActor)
            return false;
        const float halfZ = physicActor->getHalfExtents().z();
        const osg::Vec3f actorPosition = physicActor->getPosition();
        const osg::Vec3f startingPosition(actorPosition.x(), actorPosition.y(), actorPosition.z() + halfZ);
        const osg::Vec3f destinationPosition(actorPosition.x(), actorPosition.y(), waterlevel + halfZ);
        ActorTracer tracer;
        tracer.doTrace(physicActor->getCollisionObject(), startingPosition, destinationPosition, mCollisionWorld);
        return (tracer.mFraction >= 1.0f);
    }

    osg::Vec3f PhysicsSystem::getHalfExtents(const MWWorld::ConstPtr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getRenderingHalfExtents(const MWWorld::ConstPtr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getRenderingHalfExtents();
        else
            return osg::Vec3f();
    }

    osg::Vec3f PhysicsSystem::getCollisionObjectPosition(const MWWorld::ConstPtr &actor) const
    {
        const Actor* physactor = getActor(actor);
        if (physactor)
            return physactor->getCollisionObjectPosition();
        else
            return osg::Vec3f();
    }

    class ContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
    public:
        ContactTestResultCallback(const btCollisionObject* testedAgainst)
            : mTestedAgainst(testedAgainst)
        {
        }

        const btCollisionObject* mTestedAgainst;

        std::vector<MWWorld::Ptr> mResult;

        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
        {
            const btCollisionObject* collisionObject = col0Wrap->m_collisionObject;
            if (collisionObject == mTestedAgainst)
                collisionObject = col1Wrap->m_collisionObject;
            PtrHolder* holder = static_cast<PtrHolder*>(collisionObject->getUserPointer());
            if (holder)
                mResult.push_back(holder->getPtr());
            return 0.f;
        }
    };

    std::vector<MWWorld::Ptr> PhysicsSystem::getCollisions(const MWWorld::ConstPtr &ptr, int collisionGroup, int collisionMask) const
    {
        btCollisionObject* me = NULL;

        ObjectMap::const_iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
            me = found->second->getCollisionObject();
        else
            return std::vector<MWWorld::Ptr>();

        ContactTestResultCallback resultCallback (me);
        resultCallback.m_collisionFilterGroup = collisionGroup;
        resultCallback.m_collisionFilterMask = collisionMask;
        mCollisionWorld->contactTest(me, resultCallback);
        return resultCallback.mResult;
    }

    osg::Vec3f PhysicsSystem::traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, float maxHeight)
    {
        ActorMap::iterator found = mActors.find(ptr);
        if (found ==  mActors.end())
            return ptr.getRefData().getPosition().asVec3();
        else
            return MovementSolver::traceDown(ptr, position, found->second, mCollisionWorld, maxHeight);
    }

    void PhysicsSystem::addHeightField (const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject)
    {
        HeightField *heightfield = new HeightField(heights, x, y, triSize, sqrtVerts, minH, maxH, holdObject);
        mHeightFields[std::make_pair(x,y)] = heightfield;

        mCollisionWorld->addCollisionObject(heightfield->getCollisionObject(), CollisionType_HeightMap,
            CollisionType_Actor|CollisionType_Projectile);
    }

    void PhysicsSystem::removeHeightField (int x, int y)
    {
        HeightFieldMap::iterator heightfield = mHeightFields.find(std::make_pair(x,y));
        if(heightfield != mHeightFields.end())
        {
            mCollisionWorld->removeCollisionObject(heightfield->second->getCollisionObject());
            delete heightfield->second;
            mHeightFields.erase(heightfield);
        }
    }

    void PhysicsSystem::addObject (const MWWorld::Ptr& ptr, const std::string& mesh, int collisionType)
    {
        osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance = mShapeManager->getInstance(mesh);
        if (!shapeInstance || !shapeInstance->getCollisionShape())
            return;

        Object *obj = new Object(ptr, shapeInstance);
        mObjects.insert(std::make_pair(ptr, obj));

        if (obj->isAnimated())
            mAnimatedObjects.insert(obj);

        mCollisionWorld->addCollisionObject(obj->getCollisionObject(), collisionType,
                                           CollisionType_Actor|CollisionType_HeightMap|CollisionType_Projectile);
    }

    void PhysicsSystem::remove(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            mCollisionWorld->removeCollisionObject(found->second->getCollisionObject());

            if (mUnrefQueue.get())
                mUnrefQueue->push(found->second->getShapeInstance());

            mAnimatedObjects.erase(found->second);

            delete found->second;
            mObjects.erase(found);
        }

        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            delete foundActor->second;
            mActors.erase(foundActor);
        }
    }

    void PhysicsSystem::updateCollisionMapPtr(CollisionMap& map, const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        CollisionMap::iterator found = map.find(old);
        if (found != map.end())
        {
            map[updated] = found->second;
            map.erase(found);
        }

        for (CollisionMap::iterator it = map.begin(); it != map.end(); ++it)
        {
            if (it->second == old)
                it->second = updated;
        }
    }

    void PhysicsSystem::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        ObjectMap::iterator found = mObjects.find(old);
        if (found != mObjects.end())
        {
            Object* obj = found->second;
            obj->updatePtr(updated);
            mObjects.erase(found);
            mObjects.insert(std::make_pair(updated, obj));
        }

        ActorMap::iterator foundActor = mActors.find(old);
        if (foundActor != mActors.end())
        {
            Actor* actor = foundActor->second;
            actor->updatePtr(updated);
            mActors.erase(foundActor);
            mActors.insert(std::make_pair(updated, actor));
        }

        updateCollisionMapPtr(mStandingCollisions, old, updated);
    }

    Actor *PhysicsSystem::getActor(const MWWorld::Ptr &ptr)
    {
        ActorMap::iterator found = mActors.find(ptr);
        if (found != mActors.end())
            return found->second;
        return NULL;
    }

    const Actor *PhysicsSystem::getActor(const MWWorld::ConstPtr &ptr) const
    {
        ActorMap::const_iterator found = mActors.find(ptr);
        if (found != mActors.end())
            return found->second;
        return NULL;
    }

    const Object* PhysicsSystem::getObject(const MWWorld::ConstPtr &ptr) const
    {
        ObjectMap::const_iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
            return found->second;
        return NULL;
    }

    void PhysicsSystem::updateScale(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            float scale = ptr.getCellRef().getScale();
            found->second->setScale(scale);
            mCollisionWorld->updateSingleAabb(found->second->getCollisionObject());
            return;
        }
        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            foundActor->second->updateScale();
            mCollisionWorld->updateSingleAabb(foundActor->second->getCollisionObject());
            return;
        }
    }

    void PhysicsSystem::updateRotation(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            found->second->setRotation(toBullet(ptr.getRefData().getBaseNode()->getAttitude()));
            mCollisionWorld->updateSingleAabb(found->second->getCollisionObject());
            return;
        }
        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            if (!foundActor->second->isRotationallyInvariant())
            {
                foundActor->second->updateRotation();
                mCollisionWorld->updateSingleAabb(foundActor->second->getCollisionObject());
            }
            return;
        }
    }

    void PhysicsSystem::updatePosition(const MWWorld::Ptr &ptr)
    {
        ObjectMap::iterator found = mObjects.find(ptr);
        if (found != mObjects.end())
        {
            found->second->setOrigin(toBullet(ptr.getRefData().getPosition().asVec3()));
            mCollisionWorld->updateSingleAabb(found->second->getCollisionObject());
            return;
        }
        ActorMap::iterator foundActor = mActors.find(ptr);
        if (foundActor != mActors.end())
        {
            foundActor->second->updatePosition();
            mCollisionWorld->updateSingleAabb(foundActor->second->getCollisionObject());
            return;
        }
    }

    void PhysicsSystem::addActor (const MWWorld::Ptr& ptr, const std::string& mesh) {
        osg::ref_ptr<const Resource::BulletShape> shape = mShapeManager->getShape(mesh);
        if (!shape)
            return;

        Actor* actor = new Actor(ptr, shape, mCollisionWorld);
        mActors.insert(std::make_pair(ptr, actor));
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
        ActorMap::iterator found = mActors.find(MWMechanics::getPlayer());
        if (found != mActors.end())
        {
            bool cmode = found->second->getCollisionMode();
            cmode = !cmode;
            found->second->enableCollisionMode(cmode);
            found->second->enableCollisionBody(cmode);
            return cmode;
        }

        return false;
    }

    void PhysicsSystem::queueObjectMovement(const MWWorld::Ptr &ptr, const osg::Vec3f &movement)
    {
        PtrVelocityList::iterator iter = mMovementQueue.begin();
        for(;iter != mMovementQueue.end();++iter)
        {
            if(iter->first == ptr)
            {
                iter->second = movement;
                return;
            }
        }

        mMovementQueue.push_back(std::make_pair(ptr, movement));
    }

    void PhysicsSystem::clearQueuedMovement()
    {
        mMovementQueue.clear();
        mStandingCollisions.clear();
    }

    const PtrVelocityList& PhysicsSystem::applyQueuedMovement(float dt)
    {
        mMovementResults.clear();

        mTimeAccum += dt;

        const int maxAllowedSteps = 20;
        int numSteps = mTimeAccum / (mPhysicsDt);
        numSteps = std::min(numSteps, maxAllowedSteps);

        mTimeAccum -= numSteps * mPhysicsDt;

        if (numSteps)
        {
            // Collision events should be available on every frame
            mStandingCollisions.clear();
        }

        const MWBase::World *world = MWBase::Environment::get().getWorld();
        PtrVelocityList::iterator iter = mMovementQueue.begin();
        for(;iter != mMovementQueue.end();++iter)
        {
            ActorMap::iterator foundActor = mActors.find(iter->first);
            if (foundActor == mActors.end()) // actor was already removed from the scene
                continue;
            Actor* physicActor = foundActor->second;

            float waterlevel = -std::numeric_limits<float>::max();
            const MWWorld::CellStore *cell = iter->first.getCell();
            if(cell->getCell()->hasWater())
                waterlevel = cell->getWaterLevel();

            const MWMechanics::MagicEffects& effects = iter->first.getClass().getCreatureStats(iter->first).getMagicEffects();

            bool waterCollision = false;
            if (cell->getCell()->hasWater() && effects.get(ESM::MagicEffect::WaterWalking).getMagnitude())
            {
                if (!world->isUnderwater(iter->first.getCell(), osg::Vec3f(iter->first.getRefData().getPosition().asVec3())))
                    waterCollision = true;
                else if (physicActor->getCollisionMode() && canMoveToWaterSurface(iter->first, waterlevel))
                {
                    const osg::Vec3f actorPosition = physicActor->getPosition();
                    physicActor->setPosition(osg::Vec3f(actorPosition.x(), actorPosition.y(), waterlevel));
                    waterCollision = true;
                }
            }
            physicActor->setCanWaterWalk(waterCollision);

            // Slow fall reduces fall speed by a factor of (effect magnitude / 200)
            float slowFall = 1.f - std::max(0.f, std::min(1.f, effects.get(ESM::MagicEffect::SlowFall).getMagnitude() * 0.005f));

            bool flying = world->isFlying(iter->first);

            bool wasOnGround = physicActor->getOnGround();
            osg::Vec3f position = physicActor->getPosition();
            float oldHeight = position.z();
            bool positionChanged = false;
            for (int i=0; i<numSteps; ++i)
            {
                position = MovementSolver::move(position, physicActor->getPtr(), physicActor, iter->second, mPhysicsDt,
                                                flying, waterlevel, slowFall, mCollisionWorld, mStandingCollisions);
                if (position != physicActor->getPosition())
                    positionChanged = true;
                physicActor->setPosition(position); // always set even if unchanged to make sure interpolation is correct
            }
            if (positionChanged)
                mCollisionWorld->updateSingleAabb(physicActor->getCollisionObject());

            float interpolationFactor = mTimeAccum / mPhysicsDt;
            osg::Vec3f interpolated = position * interpolationFactor + physicActor->getPreviousPosition() * (1.f - interpolationFactor);

            float heightDiff = position.z() - oldHeight;

            MWMechanics::CreatureStats& stats = iter->first.getClass().getCreatureStats(iter->first);
            if ((wasOnGround && physicActor->getOnGround()) || flying || world->isSwimming(iter->first) || slowFall < 1)
                stats.land();
            else if (heightDiff < 0)
                stats.addToFallHeight(-heightDiff);

            mMovementResults.push_back(std::make_pair(iter->first, interpolated));
        }

        mMovementQueue.clear();

        return mMovementResults;
    }

    void PhysicsSystem::stepSimulation(float dt)
    {
        for (std::set<Object*>::iterator it = mAnimatedObjects.begin(); it != mAnimatedObjects.end(); ++it)
            (*it)->animateCollisionShapes(mCollisionWorld);

#ifndef BT_NO_PROFILE
        CProfileManager::Reset();
        CProfileManager::Increment_Frame_Counter();
#endif
    }

    void PhysicsSystem::debugDraw()
    {
        if (mDebugDrawer.get())
            mDebugDrawer->step();
    }

    bool PhysicsSystem::isActorStandingOn(const MWWorld::Ptr &actor, const MWWorld::ConstPtr &object) const
    {
        for (CollisionMap::const_iterator it = mStandingCollisions.begin(); it != mStandingCollisions.end(); ++it)
        {
            if (it->first == actor && it->second == object)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsStandingOn(const MWWorld::ConstPtr &object, std::vector<MWWorld::Ptr> &out) const
    {
        for (CollisionMap::const_iterator it = mStandingCollisions.begin(); it != mStandingCollisions.end(); ++it)
        {
            if (it->second == object)
                out.push_back(it->first);
        }
    }

    bool PhysicsSystem::isActorCollidingWith(const MWWorld::Ptr &actor, const MWWorld::ConstPtr &object) const
    {
        std::vector<MWWorld::Ptr> collisions = getCollisions(object, CollisionType_World, CollisionType_Actor);
        return (std::find(collisions.begin(), collisions.end(), actor) != collisions.end());
    }

    void PhysicsSystem::getActorsCollidingWith(const MWWorld::ConstPtr &object, std::vector<MWWorld::Ptr> &out) const
    {
        std::vector<MWWorld::Ptr> collisions = getCollisions(object, CollisionType_World, CollisionType_Actor);
        out.insert(out.end(), collisions.begin(), collisions.end());
    }

    void PhysicsSystem::disableWater()
    {
        if (mWaterEnabled)
        {
            mWaterEnabled = false;
            updateWater();
        }
    }

    void PhysicsSystem::enableWater(float height)
    {
        if (!mWaterEnabled || mWaterHeight != height)
        {
            mWaterEnabled = true;
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::setWaterHeight(float height)
    {
        if (mWaterHeight != height)
        {
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::updateWater()
    {
        if (mWaterCollisionObject.get())
        {
            mCollisionWorld->removeCollisionObject(mWaterCollisionObject.get());
        }

        if (!mWaterEnabled)
        {
            mWaterCollisionObject.reset();
            return;
        }

        mWaterCollisionObject.reset(new btCollisionObject());
        mWaterCollisionShape.reset(new btStaticPlaneShape(btVector3(0,0,1), mWaterHeight));
        mWaterCollisionObject->setCollisionShape(mWaterCollisionShape.get());
        mCollisionWorld->addCollisionObject(mWaterCollisionObject.get(), CollisionType_Water,
                                                    CollisionType_Actor);
    }
}
