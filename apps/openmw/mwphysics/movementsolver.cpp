#include "movementsolver.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

#include <components/esm/loadgmst.hpp>
#include <components/misc/convert.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/refdata.hpp"

#include "actor.hpp"
#include "collisiontype.hpp"
#include "constants.hpp"
#include "stepper.hpp"
#include "trace.h"

namespace MWPhysics
{
    static bool isActor(const btCollisionObject *obj)
    {
        assert(obj);
        return obj->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor;
    }

    class DeepestContactResultCallback : public btCollisionWorld::ContactResultCallback
    {
    public:
        DeepestContactResultCallback(const btCollisionObject * me) : mMe(me)
        {
            m_collisionFilterGroup = me->getBroadphaseHandle()->m_collisionFilterGroup;
            m_collisionFilterMask = me->getBroadphaseHandle()->m_collisionFilterMask;
        }
        virtual btScalar addSingleResult(btManifoldPoint & contact, const btCollisionObjectWrapper * colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper * colObj1Wrap, int partId1, int index1)
        {
            if (isActor(colObj0Wrap->getCollisionObject()) && isActor(colObj1Wrap->getCollisionObject()))
                return 0.0;
            if (contact.m_distance1 < mDistance)
            {
                mDistance = contact.m_distance1;
                mNormal = contact.m_normalWorldOnB;
                return mDistance;
            }
            return 0.0;
        }
        btVector3 mNormal{0.0, 0.0, 0.0};
        btScalar mDistance = 0.0;
    protected:
        const btCollisionObject * mMe;
    };

    osg::Vec3f MovementSolver::traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, Actor* actor, btCollisionWorld* collisionWorld, float maxHeight)
    {
        osg::Vec3f offset = actor->getCollisionObjectPosition() - ptr.getRefData().getPosition().asVec3();

        ActorTracer tracer;
        tracer.findGround(actor, position + offset, position + offset - osg::Vec3f(0,0,maxHeight), collisionWorld);
        if (tracer.mFraction >= 1.0f)
        {
            actor->setOnGround(false);
            return position;
        }

        actor->setOnGround(true);

        // Check if we actually found a valid spawn point (use an infinitely thin ray this time).
        // Required for some broken door destinations in Morrowind.esm, where the spawn point
        // intersects with other geometry if the actor's base is taken into account
        btVector3 from = Misc::Convert::toBullet(position);
        btVector3 to = from - btVector3(0,0,maxHeight);

        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        resultCallback1.m_collisionFilterGroup = 0xff;
        resultCallback1.m_collisionFilterMask = CollisionType_World|CollisionType_HeightMap;

        collisionWorld->rayTest(from, to, resultCallback1);

        if (resultCallback1.hasHit() && ((Misc::Convert::toOsg(resultCallback1.m_hitPointWorld) - tracer.mEndPos + offset).length2() > 35*35
            || !isWalkableSlope(tracer.mPlaneNormal)))
        {
            actor->setOnSlope(!isWalkableSlope(resultCallback1.m_hitNormalWorld));
            return Misc::Convert::toOsg(resultCallback1.m_hitPointWorld) + osg::Vec3f(0.f, 0.f, sGroundOffset);
        }

        actor->setOnSlope(!isWalkableSlope(tracer.mPlaneNormal));

        return tracer.mEndPos-offset + osg::Vec3f(0.f, 0.f, sGroundOffset);
    }

    osg::Vec3f MovementSolver::move(osg::Vec3f position, const MWWorld::Ptr &ptr, Actor* physicActor, const osg::Vec3f &movement, float time,
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

        static const float fSwimHeightScale = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fSwimHeightScale")->mValue.getFloat();
        float swimlevel = waterlevel + halfExtents.z() - (physicActor->getRenderingHalfExtents().z() * 2 * fSwimHeightScale);

        ActorTracer tracer;

        osg::Vec3f inertia = physicActor->getInertialForce();
        osg::Vec3f velocity;

        if (position.z() < swimlevel || isFlying)
        {
            velocity = (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) * osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * movement;
        }
        else
        {
            velocity = (osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * movement;

            if ((velocity.z() > 0.f && physicActor->getOnGround() && !physicActor->getOnSlope())
            || (velocity.z() > 0.f && velocity.z() + inertia.z() <= -velocity.z() && physicActor->getOnSlope()))
                inertia = velocity;
            else if (!physicActor->getOnGround() || physicActor->getOnSlope())
                velocity = velocity + inertia;
        }

        // dead actors underwater will float to the surface, if the CharacterController tells us to do so
        if (movement.z() > 0 && ptr.getClass().getCreatureStats(ptr).isDead() && position.z() < swimlevel)
            velocity = osg::Vec3f(0,0,1) * 25;

        if (ptr.getClass().getMovementSettings(ptr).mPosition[2])
        {
            const bool isPlayer = (ptr == MWMechanics::getPlayer());
            // Advance acrobatics and set flag for GetPCJumping
            if (isPlayer)
            {
                ptr.getClass().skillUsageSucceeded(ptr, ESM::Skill::Acrobatics, 0);
                MWBase::Environment::get().getWorld()->getPlayer().setJumping(true);
            }

            // Decrease fatigue
            if (!isPlayer || !MWBase::Environment::get().getWorld()->getGodModeState())
            {
                const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
                const float fFatigueJumpBase = gmst.find("fFatigueJumpBase")->mValue.getFloat();
                const float fFatigueJumpMult = gmst.find("fFatigueJumpMult")->mValue.getFloat();
                const float normalizedEncumbrance = std::min(1.f, ptr.getClass().getNormalizedEncumbrance(ptr));
                const float fatigueDecrease = fFatigueJumpBase + normalizedEncumbrance * fFatigueJumpMult;
                MWMechanics::DynamicStat<float> fatigue = ptr.getClass().getCreatureStats(ptr).getFatigue();
                fatigue.setCurrent(fatigue.getCurrent() - fatigueDecrease);
                ptr.getClass().getCreatureStats(ptr).setFatigue(fatigue);
            }
            ptr.getClass().getMovementSettings(ptr).mPosition[2] = 0;
        }

        // Now that we have the effective movement vector, apply wind forces to it
        if (MWBase::Environment::get().getWorld()->isInStorm())
        {
            osg::Vec3f stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
            float angleDegrees = osg::RadiansToDegrees(std::acos(stormDirection * velocity / (stormDirection.length() * velocity.length())));
            static const float fStromWalkMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fStromWalkMult")->mValue.getFloat();
            velocity *= 1.f-(fStromWalkMult * (angleDegrees/180.f));
        }
        
        // try to pop outside of the world before doing anything else if we're inside of it
        DeepestContactResultCallback contactCallback{physicActor->getCollisionObject()};
        const_cast<btCollisionWorld*>(collisionWorld)->contactTest(physicActor->getCollisionObject(), contactCallback);
        if(contactCallback.mDistance < 0.0)
        {
            bool giveup = false;
            auto tempPosition = physicActor->getPosition();
            printf("%f\n", contactCallback.mDistance);
            auto delta = contactCallback.mNormal*contactCallback.mDistance;
            auto positionDelta = Misc::Convert::toOsg(contactCallback.mNormal*contactCallback.mDistance);
            physicActor->setPosition(position - positionDelta);
            
            DeepestContactResultCallback contactCallback2{physicActor->getCollisionObject()};
            const_cast<btCollisionWorld*>(collisionWorld)->contactTest(physicActor->getCollisionObject(), contactCallback2);
            // resulting position has more penetration than we started with. try moving straight up instead
            if(contactCallback2.mDistance < contactCallback.mDistance)
            {
                physicActor->setPosition(position + osg::Vec3f(0.0, 0.0, delta.z()));
                
                DeepestContactResultCallback contactCallback3{physicActor->getCollisionObject()};
                const_cast<btCollisionWorld*>(collisionWorld)->contactTest(physicActor->getCollisionObject(), contactCallback3);
                // try again but with a fixed distance
                if(contactCallback3.mDistance < contactCallback.mDistance)
                {
                    if(contactCallback3.mDistance < -10)
                    {
                        physicActor->setPosition(position + osg::Vec3f(0.0, 0.0, 10));
                        
                        DeepestContactResultCallback contactCallback4{physicActor->getCollisionObject()};
                        const_cast<btCollisionWorld*>(collisionWorld)->contactTest(physicActor->getCollisionObject(), contactCallback4);
                        // resulting position STILL has more penetration, give up
                        if(contactCallback4.mDistance < contactCallback.mDistance)
                            giveup = true;
                    }
                    else
                        giveup = true;
                }
            }
            if(!giveup)
                position = physicActor->getPosition();
            physicActor->setPosition(tempPosition);
            
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
        bool seenGround = physicActor->getOnGround() && !physicActor->getOnSlope() && !isFlying;
        
        int numTimesSlid = 0;
        osg::Vec3f lastSlideNormal(0,0,1);
        osg::Vec3f lastSlideNormalFallback(0,0,1);
        bool forceGroundTest = false;
        
        for (int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
        {
            osg::Vec3f nextpos = newPosition + velocity * remainingTime;

            // If not able to fly, don't allow to swim up into the air
            if(!isFlying && nextpos.z() > swimlevel && newPosition.z() < swimlevel)
            {
                const osg::Vec3f down(0,0,-1);
                velocity = reject(velocity, down);
                // NOTE: remainingTime is unchanged before the loop continues
                continue; // velocity updated, calculate nextpos again
            }

            if((newPosition - nextpos).length2() > 0.0001)
            {
                // trace to where character would go if there were no obstructions
                tracer.doTrace(colobj, newPosition, nextpos, collisionWorld);

                // check for obstructions
                if(tracer.mFraction >= 1.0f)
                {
                    newPosition = tracer.mEndPos; // ok to move, so set newPosition
                    break;
                }
            }
            else
            {
                // The current position and next position are nearly the same, so just exit.
                // Note: Bullet can trigger an assert in debug modes if the positions
                // are the same, since that causes it to attempt to normalize a zero
                // length vector (which can also happen with nearly identical vectors, since
                // precision can be lost due to any math Bullet does internally). Since we
                // aren't performing any collision detection, we want to reject the next
                // position, so that we don't slowly move inside another object.
                break;
            }

            if (isWalkableSlope(tracer.mPlaneNormal) && !isFlying && newPosition.z() >= swimlevel)
                seenGround = true;

            // We hit something. Check if we can step up.
            float hitHeight = tracer.mHitPoint.z() - tracer.mEndPos.z() + halfExtents.z();
            osg::Vec3f oldPosition = newPosition;
            bool usedStepLogic = false;
            if (hitHeight < sStepSizeUp && !isActor(tracer.mHitObject))
            {
                // Try to step up onto it.
                // NOTE: this modifies newPosition and velocity on its own if successful
                usedStepLogic = stepper.step(newPosition, velocity, remainingTime, seenGround, iterations == 0);
            }
            if (usedStepLogic)
            {
                // don't let pure water creatures move out of water after stepMove
                if (ptr.getClass().isPureWaterCreature(ptr) && newPosition.z() + halfExtents.z() > waterlevel)
                    newPosition = oldPosition;
                else if(!isFlying && position.z() >= swimlevel)
                    forceGroundTest = true;
            }
            else
            {
                // Can't step up, so slide against what we ran into
                remainingTime *= (1.0f-tracer.mFraction);

                auto planeNormal = tracer.mPlaneNormal;

                // If we touched the ground this frame, and whatever we ran into is a wall of some sort,
                // pretend that its collision normal is pointing horizontally
                // (fixes snagging on slightly downward-facing walls, and crawling up the bases of very steep walls because of the collision margin)
                if (seenGround && !isWalkableSlope(planeNormal) && planeNormal.z() != 0)
                {
                    planeNormal.z() = 0;
                    planeNormal.normalize();
                }

                // Move up to what we ran into (with a bit of a collision margin)
                if ((newPosition-tracer.mEndPos).length2() > sCollisionMargin*sCollisionMargin)
                {
                    auto direction = velocity;
                    direction.normalize();
                    newPosition = tracer.mEndPos;
                    newPosition -= direction*sCollisionMargin;
                }

                osg::Vec3f newVelocity = (velocity * planeNormal <= 0.0) ? reject(velocity, planeNormal) : velocity;
                bool usedSeamLogic = false;

                // check for the current and previous collision planes forming an acute angle; slide along the seam if they do
                if(numTimesSlid > 0)
                {
                    auto dotA = lastSlideNormal * planeNormal;
                    auto dotB = lastSlideNormalFallback * planeNormal;
                    if(numTimesSlid <= 1) // ignore fallback normal if this is only the first or second slide
                        dotB = 1.0;
                    if(dotA <= 0.0 || dotB <= 0.0)
                    {
                        osg::Vec3f bestNormal = lastSlideNormal;
                        // use previous-to-previous collision plane if it's acute with current plane but actual previous plane isn't
                        if(dotB < dotA)
                        {
                            bestNormal = lastSlideNormalFallback;
                            lastSlideNormal = lastSlideNormalFallback;
                        }
                        
                        auto constraintVector = bestNormal ^ planeNormal; // cross product
                        if(constraintVector.length2() > 0) // only if it's not zero length
                        {
                            constraintVector.normalize();
                            newVelocity = project(velocity, constraintVector);
                            
                            // version of surface rejection for acute crevices/seams
                            auto averageNormal = bestNormal + planeNormal;
                            averageNormal.normalize();
                            tracer.doTrace(colobj, newPosition, newPosition + averageNormal*(sCollisionMargin*2.0), collisionWorld);
                            newPosition = (newPosition + tracer.mEndPos)/2.0;
                            
                            usedSeamLogic = true;
                        }
                    }
                }
                // otherwise just keep the normal vector rejection

                // if this isn't the first iteration, or if the first iteration is also the last iteration,
                // move away from the collision plane slightly, if possible
                // this reduces getting stuck in some concave geometry, like the gaps above the railings in some ald'ruhn buildings
                // this is different from the normal collision margin, because the normal collision margin is along the movement path,
                // but this is along the collision normal
                if(!usedSeamLogic && (iterations > 0 || remainingTime < 0.01f))
                {
                    tracer.doTrace(colobj, newPosition, newPosition + planeNormal*(sCollisionMargin*2.0), collisionWorld);
                    newPosition = (newPosition + tracer.mEndPos)/2.0;
                }

                // Do not allow sliding up steep slopes if there is gravity.
                if (newPosition.z() >= swimlevel && !isFlying && !isWalkableSlope(planeNormal))
                    newVelocity.z() = std::min(newVelocity.z(), velocity.z());

                if (newVelocity * origVelocity <= 0.0f)
                    break;

                numTimesSlid += 1;
                lastSlideNormalFallback = lastSlideNormal;
                lastSlideNormal = planeNormal;
                velocity = newVelocity;
            }
        }

        bool isOnGround = false;
        bool isOnSlope = false;
        if (forceGroundTest || (inertia.z() <= 0.f && newPosition.z() >= swimlevel))
        {
            osg::Vec3f from = newPosition;
            osg::Vec3f to = newPosition - (physicActor->getOnGround() ? osg::Vec3f(0,0,sStepSizeDown + 2*sGroundOffset) : osg::Vec3f(0,0,2*sGroundOffset));
            tracer.doTrace(colobj, from, to, collisionWorld);
            if(tracer.mFraction < 1.0f && !isActor(tracer.mHitObject))
            {
                isOnGround = true;
                isOnSlope = !isWalkableSlope(tracer.mPlaneNormal);
                
                const btCollisionObject* standingOn = tracer.mHitObject;
                PtrHolder* ptrHolder = static_cast<PtrHolder*>(standingOn->getUserPointer());
                if (ptrHolder)
                    standingCollisionTracker[ptr] = ptrHolder->getPtr();

                if (standingOn->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Water)
                    physicActor->setWalkingOnWater(true);
                if (!isFlying && !isOnSlope)
                {
                    if (tracer.mEndPos.z()+sGroundOffset <= newPosition.z())
                        newPosition.z() = tracer.mEndPos.z() + sGroundOffset;
                    else
                    {
                        newPosition.z() = tracer.mEndPos.z();
                        tracer.doTrace(colobj, newPosition, newPosition - osg::Vec3f(0, 0, 2*sGroundOffset), collisionWorld);
                        newPosition = (newPosition+tracer.mEndPos)/2.0;
                    }
                }
            }
            else
            {
                // Vanilla allows actors to over on top of other actors.
                if (tracer.mEndPos.z()+sGroundOffset <= newPosition.z())
                    newPosition.z() = tracer.mEndPos.z() + sGroundOffset;

                isOnGround = false;
            }
        }

        if((isOnGround && !isOnSlope) || newPosition.z() < swimlevel || isFlying)
            physicActor->setInertialForce(osg::Vec3f(0.f, 0.f, 0.f));
        else
        {
            inertia.z() -= time * Constants::GravityConst * Constants::UnitsPerMeter;
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
}
