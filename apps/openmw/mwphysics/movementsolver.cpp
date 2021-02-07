#include "movementsolver.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

#include <components/esm/loadgmst.hpp>
#include <components/misc/convert.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/refdata.hpp"

#include "actor.hpp"
#include "collisiontype.hpp"
#include "constants.hpp"
#include "contacttestwrapper.h"
#include "physicssystem.hpp"
#include "stepper.hpp"
#include "trace.h"

#include <cmath>

namespace MWPhysics
{
    static bool isActor(const btCollisionObject *obj)
    {
        assert(obj);
        return obj->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Actor;
    }

    class ContactCollectionCallback : public btCollisionWorld::ContactResultCallback
    {
    public:
        ContactCollectionCallback(const btCollisionObject * me, osg::Vec3f velocity) : mMe(me)
        {
            m_collisionFilterGroup = me->getBroadphaseHandle()->m_collisionFilterGroup;
            m_collisionFilterMask = me->getBroadphaseHandle()->m_collisionFilterMask & ~CollisionType_Projectile;
            mVelocity = Misc::Convert::toBullet(velocity);
        }
        btScalar addSingleResult(btManifoldPoint & contact, const btCollisionObjectWrapper * colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper * colObj1Wrap, int partId1, int index1) override
        {
            if (isActor(colObj0Wrap->getCollisionObject()) && isActor(colObj1Wrap->getCollisionObject()))
                return 0.0;
            // ignore overlap if we're moving in the same direction as it would push us out (don't change this to >=, that would break detection when not moving)
            if (contact.m_normalWorldOnB.dot(mVelocity) > 0.0)
                return 0.0;
            auto delta = contact.m_normalWorldOnB * -contact.m_distance1;
            mContactSum += delta;
            mMaxX = std::max(std::abs(delta.x()), mMaxX);
            mMaxY = std::max(std::abs(delta.y()), mMaxY);
            mMaxZ = std::max(std::abs(delta.z()), mMaxZ);
            if (contact.m_distance1 < mDistance)
            {
                mDistance = contact.m_distance1;
                mNormal = contact.m_normalWorldOnB;
                mDelta = delta;
                return mDistance;
            }
            else
            {
                return 0.0;
            }
        }
        btScalar mMaxX = 0.0;
        btScalar mMaxY = 0.0;
        btScalar mMaxZ = 0.0;
        btVector3 mContactSum{0.0, 0.0, 0.0};
        btVector3 mNormal{0.0, 0.0, 0.0}; // points towards "me"
        btVector3 mDelta{0.0, 0.0, 0.0}; // points towards "me"
        btScalar mDistance = 0.0; // negative or zero
    protected:
        btVector3 mVelocity;
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

    void MovementSolver::move(ActorFrameData& actor, float time, const btCollisionWorld* collisionWorld,
                                           WorldFrameData& worldData)
    {
        auto* physicActor = actor.mActorRaw;
        const ESM::Position& refpos = actor.mRefpos;
        // Early-out for totally static creatures
        // (Not sure if gravity should still apply?)
        {
            const auto ptr = physicActor->getPtr();
            if (!ptr.getClass().isMobile(ptr))
                return;
        }

        // Reset per-frame data
        physicActor->setWalkingOnWater(false);
        // Anything to collide with?
        if(!physicActor->getCollisionMode())
        {
            actor.mPosition += (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) *
                                osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))
                                ) * actor.mMovement * time;
            return;
        }

        const btCollisionObject *colobj = physicActor->getCollisionObject();

        // Adjust for collision mesh offset relative to actor's "location"
        // (doTrace doesn't take local/interior collision shape translation into account, so we have to do it on our own)
        // for compatibility with vanilla assets, we have to derive this from the vertical half extent instead of from internal hull translation
        // if not for this hack, the "correct" collision hull position would be physicActor->getScaledMeshTranslation()
        osg::Vec3f halfExtents = physicActor->getHalfExtents();
        actor.mPosition.z() += halfExtents.z(); // vanilla-accurate

        static const float fSwimHeightScale = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fSwimHeightScale")->mValue.getFloat();
        float swimlevel = actor.mWaterlevel + halfExtents.z() - (physicActor->getRenderingHalfExtents().z() * 2 * fSwimHeightScale);

        ActorTracer tracer;

        osg::Vec3f inertia = physicActor->getInertialForce();
        osg::Vec3f velocity;

        if (actor.mPosition.z() < swimlevel || actor.mFlying)
        {
            velocity = (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) * osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * actor.mMovement;
        }
        else
        {
            velocity = (osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * actor.mMovement;

            if ((velocity.z() > 0.f && physicActor->getOnGround() && !physicActor->getOnSlope())
            || (velocity.z() > 0.f && velocity.z() + inertia.z() <= -velocity.z() && physicActor->getOnSlope()))
                inertia = velocity;
            else if (!physicActor->getOnGround() || physicActor->getOnSlope())
                velocity = velocity + inertia;
        }

        // Dead and paralyzed actors underwater will float to the surface,
        // if the CharacterController tells us to do so
        if (actor.mMovement.z() > 0 && actor.mFloatToSurface && actor.mPosition.z() < swimlevel)
            velocity = osg::Vec3f(0,0,1) * 25;

        if (actor.mWantJump)
            actor.mDidJump = true;

        // Now that we have the effective movement vector, apply wind forces to it
        if (worldData.mIsInStorm)
        {
            osg::Vec3f stormDirection = worldData.mStormDirection;
            float angleDegrees = osg::RadiansToDegrees(std::acos(stormDirection * velocity / (stormDirection.length() * velocity.length())));
            static const float fStromWalkMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fStromWalkMult")->mValue.getFloat();
            velocity *= 1.f-(fStromWalkMult * (angleDegrees/180.f));
        }

        Stepper stepper(collisionWorld, colobj);
        osg::Vec3f origVelocity = velocity;
        osg::Vec3f newPosition = actor.mPosition;
        /*
         * A loop to find newPosition using tracer, if successful different from the starting position.
         * nextpos is the local variable used to find potential newPosition, using velocity and remainingTime
         * The initial velocity was set earlier (see above).
        */
        float remainingTime = time;
        bool seenGround = physicActor->getOnGround() && !physicActor->getOnSlope() && !actor.mFlying;

        int numTimesSlid = 0;
        osg::Vec3f lastSlideNormal(0,0,1);
        osg::Vec3f lastSlideNormalFallback(0,0,1);
        bool forceGroundTest = false;

        for (int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
        {
            osg::Vec3f nextpos = newPosition + velocity * remainingTime;

            // If not able to fly, don't allow to swim up into the air
            if(!actor.mFlying && nextpos.z() > swimlevel && newPosition.z() < swimlevel)
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

            if (isWalkableSlope(tracer.mPlaneNormal) && !actor.mFlying && newPosition.z() >= swimlevel)
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
                const auto ptr = physicActor->getPtr();
                if (ptr.getClass().isPureWaterCreature(ptr) && newPosition.z() + halfExtents.z() > actor.mWaterlevel)
                    newPosition = oldPosition;
                else if(!actor.mFlying && actor.mPosition.z() >= swimlevel)
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
                if (newPosition.z() >= swimlevel && !actor.mFlying && !isWalkableSlope(planeNormal))
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
            auto dropDistance = 2*sGroundOffset + (physicActor->getOnGround() ? sStepSizeDown : 0);
            osg::Vec3f to = newPosition - osg::Vec3f(0,0,dropDistance);
            tracer.doTrace(colobj, from, to, collisionWorld);
            if(tracer.mFraction < 1.0f)
            {
                if (!isActor(tracer.mHitObject))
                {
                    isOnGround = true;
                    isOnSlope = !isWalkableSlope(tracer.mPlaneNormal);

                    const btCollisionObject* standingOn = tracer.mHitObject;
                    PtrHolder* ptrHolder = static_cast<PtrHolder*>(standingOn->getUserPointer());
                    if (ptrHolder)
                        actor.mStandingOn = ptrHolder->getPtr();

                    if (standingOn->getBroadphaseHandle()->m_collisionFilterGroup == CollisionType_Water)
                        physicActor->setWalkingOnWater(true);
                    if (!actor.mFlying && !isOnSlope)
                    {
                        if (tracer.mFraction*dropDistance > sGroundOffset)
                            newPosition.z() = tracer.mEndPos.z() + sGroundOffset;
                        else
                        {
                            newPosition.z() = tracer.mEndPos.z();
                            tracer.doTrace(colobj, newPosition, newPosition + osg::Vec3f(0, 0, 2*sGroundOffset), collisionWorld);
                            newPosition = (newPosition+tracer.mEndPos)/2.0;
                        }
                    }
                }
                else
                {
                    // Vanilla allows actors to float on top of other actors. Do not push them off.
                    if (!actor.mFlying && isWalkableSlope(tracer.mPlaneNormal) && tracer.mEndPos.z()+sGroundOffset <= newPosition.z())
                        newPosition.z() = tracer.mEndPos.z() + sGroundOffset;

                    isOnGround = false;
                }
            }
        }

        if((isOnGround && !isOnSlope) || newPosition.z() < swimlevel || actor.mFlying)
            physicActor->setInertialForce(osg::Vec3f(0.f, 0.f, 0.f));
        else
        {
            inertia.z() -= time * Constants::GravityConst * Constants::UnitsPerMeter;
            if (inertia.z() < 0)
                inertia.z() *= actor.mSlowFall;
            if (actor.mSlowFall < 1.f) {
                inertia.x() *= actor.mSlowFall;
                inertia.y() *= actor.mSlowFall;
            }
            physicActor->setInertialForce(inertia);
        }
        physicActor->setOnGround(isOnGround);
        physicActor->setOnSlope(isOnSlope);

        actor.mPosition = newPosition;
        // remove what was added earlier in compensating for doTrace not taking interior transformation into account
        actor.mPosition.z() -= halfExtents.z(); // vanilla-accurate
    }

    btVector3 addMarginToDelta(btVector3 delta)
    {
        if(delta.length2() == 0.0)
            return delta;
        return delta + delta.normalized() * sCollisionMargin;
    }

    void MovementSolver::unstuck(ActorFrameData& actor, const btCollisionWorld* collisionWorld)
    {
        const auto& ptr = actor.mActorRaw->getPtr();
        if (!ptr.getClass().isMobile(ptr))
            return;

        auto* physicActor = actor.mActorRaw;
        if(!physicActor->getCollisionMode()) // noclipping/tcl
            return;

        auto* collisionObject = physicActor->getCollisionObject();
        auto tempPosition = actor.mPosition;

        // use vanilla-accurate collision hull position hack (do same hitbox offset hack as movement solver)
        // if vanilla compatibility didn't matter, the "correct" collision hull position would be physicActor->getScaledMeshTranslation()
        const auto verticalHalfExtent = osg::Vec3f(0.0, 0.0, physicActor->getHalfExtents().z());

        // use a 3d approximation of the movement vector to better judge player intent
        const ESM::Position& refpos = ptr.getRefData().getPosition();
        auto velocity = (osg::Quat(refpos.rot[0], osg::Vec3f(-1, 0, 0)) * osg::Quat(refpos.rot[2], osg::Vec3f(0, 0, -1))) * actor.mMovement;
        // try to pop outside of the world before doing anything else if we're inside of it
        if (!physicActor->getOnGround() || physicActor->getOnSlope())
                velocity += physicActor->getInertialForce();

        // because of the internal collision box offset hack, and the fact that we're moving the collision box manually,
        // we need to replicate part of the collision box's transform process from scratch
        osg::Vec3f refPosition = tempPosition + verticalHalfExtent;
        osg::Vec3f goodPosition = refPosition;
        const btTransform oldTransform = collisionObject->getWorldTransform();
        btTransform newTransform = oldTransform;

        auto gatherContacts = [&](btVector3 newOffset) -> ContactCollectionCallback
        {
            goodPosition = refPosition + Misc::Convert::toOsg(addMarginToDelta(newOffset));
            newTransform.setOrigin(Misc::Convert::toBullet(goodPosition));
            collisionObject->setWorldTransform(newTransform);

            ContactCollectionCallback callback{collisionObject, velocity};
            ContactTestWrapper::contactTest(const_cast<btCollisionWorld*>(collisionWorld), collisionObject, callback);
            return callback;
        };

        // check whether we're inside the world with our collision box with manually-derived offset
        auto contactCallback = gatherContacts({0.0, 0.0, 0.0});
        if(contactCallback.mDistance < -sAllowedPenetration)
        {
            // we are; try moving it out of the world
            auto positionDelta = contactCallback.mContactSum;
            // limit rejection delta to the largest known individual rejections
            if(std::abs(positionDelta.x()) > contactCallback.mMaxX)
                positionDelta *= contactCallback.mMaxX / std::abs(positionDelta.x());
            if(std::abs(positionDelta.y()) > contactCallback.mMaxY)
                positionDelta *= contactCallback.mMaxY / std::abs(positionDelta.y());
            if(std::abs(positionDelta.z()) > contactCallback.mMaxZ)
                positionDelta *= contactCallback.mMaxZ / std::abs(positionDelta.z());

            auto contactCallback2 = gatherContacts(positionDelta);
            // successfully moved further out from contact (does not have to be in open space, just less inside of things)
            if(contactCallback2.mDistance > contactCallback.mDistance)
                tempPosition = goodPosition - verticalHalfExtent;
            // try again but only upwards (fixes some bad coc floors)
            else
            {
                // upwards-only offset
                auto contactCallback3 = gatherContacts({0.0, 0.0, std::abs(positionDelta.z())});
                // success
                if(contactCallback3.mDistance > contactCallback.mDistance)
                    tempPosition = goodPosition - verticalHalfExtent;
                else
                // try again but fixed distance up
                {
                    auto contactCallback4 = gatherContacts({0.0, 0.0, 10.0});
                    // success
                    if(contactCallback4.mDistance > contactCallback.mDistance)
                        tempPosition = goodPosition - verticalHalfExtent;
                }
            }
        }

        collisionObject->setWorldTransform(oldTransform);
        actor.mPosition = tempPosition;
    }
}
