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

    template <class Vec3>
    static bool isWalkableSlope(const Vec3 &normal)
    {
        static const float sMaxSlopeCos = std::cos(osg::DegreesToRadians(sMaxSlope));
        return (normal.z() > sMaxSlopeCos);
    }

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

        Stepper stepper(collisionWorld, colobj);
        osg::Vec3f origVelocity = velocity;
        osg::Vec3f newPosition = position;
        /*
         * A loop to find newPosition using tracer, if successful different from the starting position.
         * nextpos is the local variable used to find potential newPosition, using velocity and remainingTime
         * The initial velocity was set earlier (see above).
        */
        float remainingTime = time;
        for (int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
        {
            osg::Vec3f nextpos = newPosition + velocity * remainingTime;

            // If not able to fly, don't allow to swim up into the air
            if(!isFlying && nextpos.z() > swimlevel && newPosition.z() < swimlevel)
            {
                const osg::Vec3f down(0,0,-1);
                velocity = slide(velocity, down);
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
            if (hitHeight < sStepSizeUp && !isActor(tracer.mHitObject))
            {
                // Try to step up onto it.
                // NOTE: stepMove does not allow stepping over, modifies newPosition if successful
                result = stepper.step(newPosition, velocity*remainingTime, remainingTime);
            }
            if (result)
            {
                // don't let pure water creatures move out of water after stepMove
                if (ptr.getClass().isPureWaterCreature(ptr) && newPosition.z() + halfExtents.z() > waterlevel)
                    newPosition = oldPosition;
            }
            else
            {
                // Can't move this way, try to find another spot along the plane
                osg::Vec3f newVelocity = slide(velocity, tracer.mPlaneNormal);

                // Do not allow sliding upward if there is gravity.
                // Stepping will have taken care of that.
                if(!(newPosition.z() < swimlevel || isFlying))
                    newVelocity.z() = std::min(newVelocity.z(), 0.0f);

                if ((newVelocity-velocity).length2() < 0.01)
                    break;
                if ((newVelocity * origVelocity) <= 0.f)
                    break; // ^ dot product

                velocity = newVelocity;
            }
        }

        bool isOnGround = false;
        bool isOnSlope = false;
        if (!(inertia.z() > 0.f) && !(newPosition.z() < swimlevel))
        {
            osg::Vec3f from = newPosition;
            osg::Vec3f to = newPosition - (physicActor->getOnGround() ? osg::Vec3f(0,0,sStepSizeDown + 2*sGroundOffset) : osg::Vec3f(0,0,2*sGroundOffset));
            tracer.doTrace(colobj, from, to, collisionWorld);
            if(tracer.mFraction < 1.0f && !isActor(tracer.mHitObject))
            {
                const btCollisionObject* standingOn = tracer.mHitObject;
                PtrHolder* ptrHolder = static_cast<PtrHolder*>(standingOn->getUserPointer());
                if (ptrHolder)
                    standingCollisionTracker[ptr] = ptrHolder->getPtr();

                if (standingOn->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Water)
                    physicActor->setWalkingOnWater(true);
                if (!isFlying)
                    newPosition.z() = tracer.mEndPos.z() + sGroundOffset;

                isOnGround = true;

                isOnSlope = !isWalkableSlope(tracer.mPlaneNormal);
            }
            else
            {
                // standing on actors is not allowed (see above).
                // in addition to that, apply a sliding effect away from the center of the actor,
                // so that we do not stay suspended in air indefinitely.
                if (tracer.mFraction < 1.0f && isActor(tracer.mHitObject))
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
